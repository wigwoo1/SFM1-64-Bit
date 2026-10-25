"""Offline map enable-request policy, original x86 versus owned x64.

Original DLL files are read-only. No SFM process, device/loader initialization,
window capture/control, memory attachment or native notification is used.
"""
from datetime import datetime, timezone
import itertools
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import verify_bsp_ambient_selection as regression
from verify_ifm_draw_packet import check_ranges

ROOT=regression.ROOT
OUT=ROOT/'outputs/map-hdr-enable-20261024'
ENGINE,SHADER,MAP=regression.ENGINE,regression.SHADER,regression.MAP
sha,require=regression.sha,regression.require


def main():
    run=Path(tempfile.mkdtemp(prefix='enable-verification-',dir=OUT))
    before={str(p):sha(p) for p in [ENGINE,SHADER,MAP]}
    require(before[str(ENGINE)]=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f','Engine pin')
    require(before[str(SHADER)]=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa','Shader pin')
    require(before[str(MAP)]=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Map pin')
    tracked=['source_bsp_hdr_enable_candidate.h','source_bsp_ambient_selection_candidate.h','probe_bsp_ambient_selection.cpp',
             'test_bsp_hdr_enable.cpp','test_bsp_ambient_selection.cpp','build_bsp_hdr_enable.cmd','verify_bsp_hdr_enable.py',
             'prepared_session_scene_candidate.h','probe_vulkan_win32_surface_x64.cpp']
    sources={p:sha(ROOT/'work'/p) for p in tracked}
    _,evidence=check_ranges(ENGINE,'engine-disassembly.txt',[
        ('map_header_call_policy_and_original_return',0x100e34a0,0x100e3556),
        ('native_enable_helper_input_byte',0x100e1e50,0x100e1e7b),
    ])
    warnings={}
    for name in ['native','owned','safety','enable-safety']:
        log=(OUT/(name+'-build.log')).read_text()
        warnings[name]=[x for x in log.splitlines() if 'warning ' in x.lower()]
        require(not warnings[name] and 'error C' not in log,'Build diagnostics '+name)
    executables=[]
    for name,machine,magic in [('selection_native_x86',0x14c,0x10b),('selection_owned_x64',0x8664,0x20b),
                               ('selection_safety_x64',0x8664,0x20b),('enable_safety_x64',0x8664,0x20b)]:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic,'PE architecture '+name)
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine),magic=hex(magic)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=60)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr)
        return p.stdout
    def compare(rows,fmt,mode,original,label,want):
        queries=run/(label+'-queries.bin');queries.write_bytes(b''.join(struct.pack(fmt,*r) for r in rows))
        answers=[]
        for name in ['selection_native_x86','selection_owned_x64']:
            output=run/(label+'-'+name+'.bin');args=[queries,output]
            if mode:args.append(mode)
            if name.endswith('x86'):args.append(original)
            execute(name,args,label+'-'+name);answers.append(output.read_bytes())
        require(answers[0]==answers[1]==want,label+' original/owned/independent mismatch')
        return answers[0]
    # Each requested effective length is tested through all8 override masks.
    # Inactive fields deliberately disagree with the active field.
    rows=[];expected=[]
    for version,lengths,level,type_,mask in itertools.product(
            [-2147483648,19,20,21,2147483647],itertools.product([-1,0,1],repeat=3),[-1,1,2,2147483647],[-1,0,1,2],range(8)):
        triples=[]
        for i,length in enumerate(lengths):
            active=(mask>>i)&1;other=17 if length<=0 else 0
            triples.extend([0x80000000 if active else 0,other if active else length,length if active else other])
        rows.append([version,*triples,level,type_])
        capable=lengths[0]>0 and lengths[1]>0 and (version<20 or lengths[2]!=0)
        calls=int(capable and level>=2)
        expected.append(struct.pack('<3I',int(capable),int(calls!=0 and type_!=0),calls))
    # Signed length boundaries and non-boolean override markers.
    for index,length,active in itertools.product(range(3),[-2147483648,2147483647],[0,1,0xffffffff]):
        lengths=[28,28,28];lengths[index]=length;triples=[]
        for i,value in enumerate(lengths):
            marker=active if i==index else 0
            triples.extend([marker,0 if marker else value,value if marker else 0])
        rows.append([20,*triples,2,2]);capable=lengths[0]>0 and lengths[1]>0 and lengths[2]!=0
        expected.append(struct.pack('<3I',int(capable),int(capable),int(capable)))
    enable_bytes=compare(rows,'<iIiiIiiIiiii','--enable-request',ENGINE,'enable',b''.join(expected))
    # Composed map -> explicit owned enabled-state handoff -> current HDR query
    # -> storage selector, using real benchmark lump lengths. This does NOT run
    # the native enabled-state notification/material lifecycle.
    raw=MAP.read_bytes();word=lambda o:struct.unpack_from('<i',raw,o)[0]
    version=word(4);leaf_version=word(8+10*16+8)
    lengths=[word(8+i*16+4) for i in [53,54,55]]
    require(version==20 and leaf_version==1 and all(x>0 for x in lengths),'Benchmark header shape')
    configs=list(itertools.product([-1,0,1,2,3],[89,90,95],[-1,0,1,2],[-1,0,1]))
    chain_enable=[];chain_enable_want=[];hardware_rows=[];hardware_want=[];selection_rows=[];selection_want=[]
    selected_hdr=0
    for level,dx,type_,emulate in configs:
        request=level>=2 and type_!=0
        chain_enable.append([version,*[v for n in lengths for v in (0,n,0)],level,type_])
        chain_enable_want.append(struct.pack('<3I',1,int(request),int(level>=2)))
    chain=compare(chain_enable,'<iIiiIiiIiiii','--enable-request',ENGINE,'chain-enable',b''.join(chain_enable_want))
    for i,(level,dx,type_,emulate) in enumerate(configs):
        requested=struct.unpack_from('<3I',chain,i*12)[1]
        enabled=1 if emulate!=0 else requested;answer=type_ if enabled and dx>=90 else 0
        hardware_rows.append([emulate,requested,dx,type_,1,0]);hardware_want.append(struct.pack('<iII',answer,enabled,enabled))
    hardware=compare(hardware_rows,'<iIiiII','--hardware-query',SHADER,'chain-hardware',b''.join(hardware_want))
    for i in range(len(configs)):
        current=struct.unpack_from('<i',hardware,i*12)[0];storage=2 if current!=0 else 1;selected_hdr+=storage==2
        selection_rows.append([leaf_version,current,0,lengths[2],0]);selection_want.append(struct.pack('<2I',storage,1))
    compare(selection_rows,'<IiIii','',ENGINE,'chain-selection',b''.join(selection_want))
    safety=execute('enable_safety_x64',[MAP],'enable-safety');require('checks=18 explicit_configurations=8' in safety,'Safety composition coverage')
    # Fresh binaries must retain all previously verified selector/query modes.
    previous_runs=set(OUT.glob('verification-*/results.json'));regression.OUT=OUT;regression.main()
    receipts=set(OUT.glob('verification-*/results.json'))-previous_runs;require(len(receipts)==1,'Regression receipt')
    old_receipt=receipts.pop();old=json.loads(old_receipt.read_text());require(old['status']=='PASS','Regression failed')
    require(all(sha(p)==h for p,h in before.items()),'Original changed')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()),'Source changed during test')
    require(all(sha(e['path'])==e['sha256'] for e in executables),'Executable changed during test')
    receipt=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),enable_request_cases=len(rows),exact_enable_bytes=len(enable_bytes),
                 composed_configurations=len(configs),composed_hdr=selected_hdr,composed_ldr=len(configs)-selected_hdr,safety=safety.strip(),
                 regression_receipt=str(old_receipt),regression_receipt_sha256=sha(old_receipt),regression_selection_cases=old['selection_cases'],
                 regression_hardware_cases=old['hardware_cases'],static_instruction_bytes=sum(e['bytes'] for e in evidence),
                 static_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in evidence],warnings=warnings,executables=executables,
                 original_sha256=before,sources_sha256=sources,benchmark_effective_lengths=dict(zip([53,54,55],lengths)),
                 limitations=[
                     'Original x86 map policy runs with header initialization stubbed; enable argument low byte is captured; postpolicy metadata/cleanup calls are skipped.',
                     'No native GPU discovery, device setup, enabled-state setter/notifications/material invalidation lifecycle is executed. State handoff is explicit owned test composition.',
                     'Signed negative length compatibility here does not admit corrupt payloads: existing BSP readers still validate file bounds and payloads independently.',
                     'External overrides are controlled policy inputs; loading external lump payload files is not implemented by this milestone.',
                     'Actual native settings, Pyro origin/bounds/pose binding, cache reuse and raster/material parity remain unverified/incomplete.',
                     'No editor dependency changed, full editor rebuild/run, GPU job, computer use/live process probe, original SFM/Blender launch or original write.',
                 ])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:receipt[k] for k in ['status','enable_request_cases','exact_enable_bytes','composed_configurations','composed_hdr','composed_ldr']}))
    print('receipt='+str(run/'results.json'))


if __name__=='__main__':main()
