"""Isolated original x86 branch/query versus owned x64 selection.

Never launches/attaches to original SFM or loads an original DLL. The oracle
maps disk bytes privately with controlled inputs and sealed executable pages.
"""
from datetime import datetime, timezone
import hashlib
import itertools
import json
from pathlib import Path
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/bsp-ambient-selection-20261024'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
ENGINE=GAME/'bin/engine.dll'
SHADER=GAME/'bin/shaderapidx9.dll'
MAP=GAME/'tf_movies/maps/mtt_soldier.bsp'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require(value,message):
    if not value:raise RuntimeError(message)


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    before={str(p):sha(p) for p in [ENGINE,SHADER,MAP]}
    require(before[str(ENGINE)]=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f','Pinned engine changed')
    prior=json.loads((ROOT/'outputs/sfm64-generic-light-hardware-queries-recovery.json').read_text())
    require(before[str(SHADER)]==prior['binary_sha256'],'Pinned hardware provider changed')
    require(before[str(MAP)]=='60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea','Pinned benchmark map changed')
    tracked=['source_bsp_ambient_selection_candidate.h','probe_bsp_ambient_selection.cpp','test_bsp_ambient_selection.cpp',
             'build_bsp_ambient_selection.cmd','verify_bsp_ambient_selection.py','source_new_record_leaf_ambient_candidate.h',
             'prepared_session_scene_candidate.h','probe_vulkan_win32_surface_x64.cpp']
    sources={name:sha(ROOT/'work'/name) for name in tracked}
    engine,ee=check_ranges(ENGINE,'engine-disassembly.txt',[
        ('leaf_version_and_hdr_branch',0x100e49b0,0x100e4a1d),
        ('hdr_lump_requests_and_loader',0x100e4a1d,0x100e4a51),
        ('ldr_lump_requests_and_loader',0x100e4a77,0x100e4aab),
        ('legacy_loader_dispatch',0x100e4ae6,0x100e4af5),
        ('effective_lump_length',0x100e1e20,0x100e1e4f),
        ('lump_helper_header_override_fields',0x100e3249,0x100e32b5),
        ('split_ambient_pointer_population',0x100e07da,0x100e085e),
        ('map_hdr_capability_and_enable_request',0x100e34b1,0x100e3545),
        ('hdr_enabled_state_get_set',0x100e1e50,0x100e1e7b),
        ('mat_hdr_level_registration',0x103098a0,0x103098c0),
    ])
    shader,se=check_ranges(SHADER,'shaderapidx9-disassembly.txt',[
        ('current_hdr_type_query',0x10003940,0x1000398b),
        ('dx_level_getter',0x100034f0,0x100034f7),
        ('hardware_hdr_type_getter',0x10003680,0x10003687),
        ('hdr_enabled_get_set',0x10003090,0x100030b0),
        ('emulate_gl_registration',0x100f9660,0x100f9676),
    ])
    require(engine.string(engine.word(0x1040e968))=='MaterialSystemHardwareConfig013' and engine.word(0x1040e96c)==0x10694194,'Engine hardware interface binding')
    for slot,target in [(17,0x100034f0),(34,0x10003940),(35,0x10003680),(46,0x10003090),(47,0x100030a0)]:
        require(shader.word(0x100fa6b4+slot*4)==target,'Hardware-config013 vtable '+str(slot))
    require(shader.string(0x100fead4)=='r_emulategl' and shader.string(0x100fa7d0)=='0','GL control registration')
    require(engine.string(0x10341d00)=='mat_hdr_level' and engine.string(0x1031eef8)=='2','HDR level registration')
    # The mapped override record uses28byte stride; base BSP descriptors16.
    require(0x105cd550+55*28==0x105cdb54 and 0x105cde68+55*16+4==0x105ce1dc,'Lump55 length address derivation')
    values=[-2147483648,-1,0,1,28,1048576,2147483647]
    rows=list(itertools.product([0,1],[-2147483648,-1,0,1,2,2147483647],[0,1,0x80000000],values,values))
    queries=run/'selection-queries.bin'
    queries.write_bytes(b''.join(struct.pack('<IiIii',*row) for row in rows))
    expected=b''.join(struct.pack('<2I',0 if version==0 else 2 if hdr!=0 and (override if active else header)>0 else 1,
                                 int(version==1)) for version,hdr,active,header,override in rows)
    executables=[]
    for name,machine in [('selection_native_x86',0x14c),('selection_owned_x64',0x8664),('selection_safety_x64',0x8664)]:
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine,'Binary architecture')
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=45)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr)
        return p.stdout
    selection_outputs=[]
    for name in ['selection_native_x86','selection_owned_x64']:
        output=run/(name+'.bin');args=[queries,output]
        if name.endswith('x86'):args.append(ENGINE)
        execute(name,args,name);selection_outputs.append(output.read_bytes())
    require(selection_outputs[0]==selection_outputs[1]==expected,'Original/owned/independent selection or call count differs')
    hardware_rows=list(itertools.product([-2147483648,-1,0,1,2],[0,1,2,255],[-1,0,89,90,91,95],[-1,0,1,2],[0,1],[0,2]))
    hardware_queries=run/'hardware-queries.bin';hardware_queries.write_bytes(b''.join(struct.pack('<iIiiII',*r) for r in hardware_rows))
    want=[]
    for emulate,enabled,dx,hdr,alias,global_enabled in hardware_rows:
        if emulate!=0:
            if alias:enabled=1
            else:global_enabled=1
        answer=hdr if enabled!=0 and dx>=90 else 0
        want.append(struct.pack('<iII',answer,enabled,enabled if alias else global_enabled))
    hardware_outputs=[]
    for name in ['selection_native_x86','selection_owned_x64']:
        output=run/('hardware-'+name+'.bin');args=[hardware_queries,output,'--hardware-query']
        if name.endswith('x86'):args.append(SHADER)
        execute(name,args,'hardware-'+name);hardware_outputs.append(output.read_bytes())
    require(hardware_outputs[0]==hardware_outputs[1]==b''.join(want),'Original/owned/independent hardware query/state differs')
    safety=execute('selection_safety_x64',[MAP],'safety')
    require('checks=17 rejections=5' in safety,'Safety coverage')
    warnings={}
    for name in ['native','owned','safety']:
        log=(OUT/(name+'-build.log')).read_text();warnings[name]=[x for x in log.splitlines() if 'warning ' in x.lower()]
        require(not warnings[name] and 'error C' not in log,'Build diagnostics require review')
    require(all(sha(p)==h for p,h in before.items()),'Original input changed')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()),'Source changed during verification')
    require(all(sha(e['path'])==e['sha256'] for e in executables),'Executable changed during verification')
    receipt=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),selection_cases=len(rows),hardware_cases=len(hardware_rows),
        exact_selection_bytes=len(expected),exact_hardware_bytes=sum(map(len,want)),safety=safety.strip(),warnings=warnings,executables=executables,
        static_instruction_bytes=sum(e['bytes'] for e in ee+se),static_ranges=dict(engine=[{k:v for k,v in e.items() if k!='instructions'} for e in ee],shader=[{k:v for k,v in e.items() if k!='instructions'} for e in se]),
        original_sha256=before,sources_sha256=sources,selection_queries_sha256=sha(queries),hardware_queries_sha256=sha(hardware_queries),
        rules=dict(interface='MaterialSystemHardwareConfig013',query_slot=34,hardware_type_slot=35,enabled_get_slot=46,enabled_set_slot=47,
                   hdr_sample_lump=55,hdr_index_lump=51,ldr_sample_lump=56,ldr_index_lump=52,
                   r_emulategl_registered_default='0',mat_hdr_level_registered_default='2',live_state='UNVERIFIED'),
        limitations=[
            'Fresh private x86 execution of original engine selection only; lump construction and path entry epilogues are controlled. Full loaders/error paths/legacy payload conversion do not execute.',
            'Fresh complete original DX9 hardware HDR query, DX-level getter and enable setter execute on controlled object fields/vtables; actual GPU capability setup/device initialization is excluded.',
            'Engine and shader DLLs use separate oracle processes because both prefer0x10000000. No DLL entrypoint/import initialization or native SFM process is used.',
            'Static map capability policy checks HDRlighting53/worldlights54, plus nonempty ambient55 for BSPversion>=20, mat_hdr_level>=2 and hardware query35 before requesting HDR enable. This entire lifecycle is not executed.',
            'Split reader still rejects unsupported versions/compression/missing tables; selection of legacy storage does not implement legacy ambient reading or full native fallback synthesis.',
            'Public SDK header is version012 while installed binding is013. Native vtable slots were derived from installed bytes and existing factory evidence, not copied from SDK order.',
            'No native live settings measured, default editor lighting changed, editor rebuilt/run, GPU job, computer use, live probing, SFM/Blender launch or original write.',
        ])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:receipt[k] for k in ['status','selection_cases','hardware_cases','static_instruction_bytes','safety']}))
    print('receipt='+str(run/'results.json'))


if __name__=='__main__':main()
