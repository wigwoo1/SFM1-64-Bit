"""Verify the benchmark sequence dependency without interacting with SFM."""
from datetime import datetime,timezone
import hashlib
import json
from pathlib import Path
import random
import re
import shutil
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA
from verify_studio_vcollide_ready import DATACACHE,DATACACHE_SHA
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/benchmark-draw-binding-20261025'
GAME=IFM.parent.parent.parent
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def require(v,m):
    if not v:raise RuntimeError(m)
def encode(groups):
    return struct.pack('<I',len(groups))+b''.join(struct.pack('<I',len(g))+b''.join(struct.pack('<III',flags,activity,len(label.encode('ascii')))+label.encode('ascii') for label,flags,activity in g) for g in groups)
def decode_output(raw,cases):
    at=0;results=[]
    for groups in cases:
        n=struct.unpack_from('<I',raw,at)[0];at+=4;entries=[struct.unpack_from('<4I',raw,at+i*16) for i in range(n)];at+=n*16;maps=[]
        for g in groups:maps.append(list(struct.unpack_from('<'+str(len(g))+'I',raw,at)));at+=len(g)*4
        results.append(dict(entries=entries,maps=maps))
    require(at==len(raw),'Output framing');return results
def metadata(path):
    b=path.read_bytes();count,start=struct.unpack_from('<ii',b,0xbc);seq=[];bounds=[]
    require(b[:8]==b'IDST1\0\0\0' and 0<=count<=4096 and 0<=start<=len(b)-count*212,'Independent MDL table')
    for i in range(count):
        at=start+i*212;label_at=at+struct.unpack_from('<i',b,at+4)[0];end=b.index(0,label_at);label=b[label_at:end].decode('ascii');flags,activity=struct.unpack_from('<II',b,at+12)
        seq.append((label,flags,activity));bounds.append(struct.unpack_from('<6f',b,at+32))
    return seq,bounds
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    originals={str(p):sha(p) for p in [IFM,DATACACHE,SESSION,GAME/'usermod/gameinfo.txt']}
    require(originals[str(IFM)]==IFM_SHA and originals[str(DATACACHE)]==DATACACHE_SHA and originals[str(SESSION)]==SESSION_SHA,'Original pins')
    sources={p:sha(ROOT/'work'/p) for p in ['source_mdl_virtual_sequences_candidate.h','probe_virtual_sequence_append.cpp','probe_virtual_sequence_lookup.cpp','test_virtual_sequence_append.cpp','audit_benchmark_draw_binding.cpp','build_benchmark_draw_binding.cmd','verify_benchmark_draw_binding.py','ifm_renderable_bounds_candidate.h','ifm_draw_frames_candidate.h','probe_vulkan_win32_surface_x64.cpp']}
    prior_editor=ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe';editor_hash=sha(prior_editor)
    require(editor_hash=='2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2','Previous editor pin')
    _,cache_evidence=check_ranges(DATACACHE,'datacache-disassembly.txt',[('complete_sequence_append',0x10013060,0x100134bf),('append_before_include_recursion',0x10013b5c,0x10013c12),('group_include_recursion',0x10013d49,0x10013d91),('ASCII_compare',0x100185b0,0x1001862e)])
    image,ifm_evidence=check_ranges(IFM,'ifm-disassembly.txt',[('complete_sequence_lookup',0x105da450,0x105da4e4),('complete_state_sync',0x1023bfb0,0x1023c096),('sequence_field_getter',0x105c2e00,0x105c2e07),('sequence_field_registration',0x105c290c,0x105c2924)])
    require(image.string(0x10ac9448)=='sequence','Native sequence attribute name')
    warnings={};executables=[]
    for tag,name,machine in [('native','sequence_native_x86',0x14c),('owned','sequence_owned_x64',0x8664),('lookup-native','sequence_lookup_native_x86',0x14c),('lookup-owned','sequence_lookup_owned_x64',0x8664),('safety','sequence_safety_x64',0x8664),('audit','benchmark_draw_binding_x64',0x8664)]:
        log=(OUT/(tag+'-build.log')).read_text();warnings[tag]=[x for x in log.splitlines() if 'warning ' in x.lower()]
        require(all(tag=='audit' and 'C4458' in x and 'graph_transform_hierarchy.h' in x for x in warnings[tag]) and 'error C' not in log,'Build diagnostics '+tag)
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'PE identity '+name)
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=90)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    safety=execute('sequence_safety_x64',[],'safety');require('checks=7 refusals=7' in safety,'Safety coverage')
    source=run/'benchmark-copy.dmx';shutil.copyfile(SESSION,source);copy_hash=sha(source)
    benchmark=run/'benchmark-queries.bin';audit=execute('benchmark_draw_binding_x64',[source,GAME/'usermod/gameinfo.txt',benchmark],'benchmark-audit')
    require('PASS benchmark_cases=4' in audit and audit.count('sequence=type2:57')==2 and audit.count('sequence=type2:0')==2,'Actual sequence fields')
    models=[];seq_by_path={};bounds_by_path={}
    for value in sorted(set(re.findall(r'^metadata_path=(.+)$',audit,re.M))):
        p=Path(value);seq,boxes=metadata(p);seq_by_path[value]=seq;bounds_by_path[value]=boxes;models.append(dict(path=value,sha256=sha(p),local_sequences=len(seq)))
    require(len(models)==3 and sorted(m['local_sequences'] for m in models)==[1,2,280],'Actual root/include counts')
    # Reconstruct the exact wire fixture independently from raw MDL descriptors,
    # not from the C++ metadata reader's selected bounds or table output.
    pyro=next(p for p in seq_by_path if p.replace('\\','/').endswith('/hwm/pyro.mdl'))
    included=next(p for p in seq_by_path if p.replace('\\','/').endswith('/pyro_animations.mdl'))
    flame=next(p for p in seq_by_path if p.replace('\\','/').endswith('/w_flamethrower.mdl'))
    benchmark_cases=[[seq_by_path[pyro],seq_by_path[included]],[seq_by_path[flame]]]*2
    require(benchmark.read_bytes()==b''.join(map(encode,benchmark_cases)),'C++ metadata differs from independent raw fields')
    rng=random.Random(2026102502)
    cases=[[[('REF',0x800,3),('fixed',1,4)],[('ref',2,7),('FIXED',0x800,8),('new',0,9),('NEW',4,10)]], [[],[]],[[('same',0,0),('SAME',0x800,2)],[('Same',7,3)]]]
    for i in range(1024):
        groups=[]
        for g in range(rng.randrange(1,6)):
            group=[]
            for j in range(rng.randrange(0,33)):
                label=rng.choice(['ref','REF','idle','Idle','run','RUN','sequence'+str(rng.randrange(64))]);group.append((label,rng.choice([0,1,0x800,0x801,0xffffffff]),rng.getrandbits(32)))
            groups.append(group)
        cases.append(groups)
    cases.extend(benchmark_cases);query=run/'queries.bin';query.write_bytes(b''.join(map(encode,cases)))
    outputs=[]
    for name in ['sequence_native_x86','sequence_owned_x64']:
        p=run/(name+'.bin');execute(name,[DATACACHE,query,p],name);outputs.append(p.read_bytes())
    require(outputs[0]==outputs[1],'Sequence append/mapping differs; preserve first mismatch')
    tables=decode_output(outputs[0],cases);require(tables[0]['entries']==[(2,7,1,0),(1,4,0,1),(0,9,1,2),(4,10,1,3)] and tables[0]['maps']==[[0,1],[0,1,2,3]],'Independent override/within-group duplicate expectations')
    # Feed ACTUAL original append results to original IFM lookup in another
    # process (both private DLL images require the same preferred base).
    lookup_rows=[];lookup_expected=[];lookup_tables=0
    for groups,t in zip(cases,tables):
        if not t['entries']:continue
        n=len(t['entries']);requests=[-2147483648,-1,0,1,57,n-1,n,n+1,2147483647]+list(range(n))
        lookup_rows.append(struct.pack('<I',len(groups))+struct.pack('<'+str(len(groups))+'I',*map(len,groups))+struct.pack('<I',n)+b''.join(struct.pack('<4I',*e) for e in t['entries'])+struct.pack('<I',len(requests))+struct.pack('<'+str(len(requests))+'i',*requests))
        for index in requests:lookup_expected.append(struct.pack('<2I',*t['entries'][index if 0<=index<n else 0][2:]))
        lookup_tables+=1
    lookup_query=run/'lookup-queries.bin';lookup_query.write_bytes(b''.join(lookup_rows));lookup_outputs=[]
    for name in ['sequence_lookup_native_x86','sequence_lookup_owned_x64']:
        p=run/(name+'.bin');execute(name,[IFM,lookup_query,p],name);lookup_outputs.append(p.read_bytes())
    require(lookup_outputs[0]==lookup_outputs[1]==b''.join(lookup_expected),'Original append -> IFM lookup composition differs')
    selections=[]
    for i,t in enumerate(tables[-4:]):
        sequence=57 if i%2==0 else 0;paths=[pyro,included] if i%2==0 else [flame];entry=t['entries'][sequence];flags,activity,group,local=entry;path=paths[group]
        label=seq_by_path[path][local][0];bounds=bounds_by_path[path][local]
        selections.append(dict(tick=[50000,50000,50417,50417][i],model=13207 if i%2==0 else 13817,session_sequence=sequence,virtual_sequences=len(t['entries']),group=group,local_sequence=local,label=label,bounds=bounds,source=path))
        require('selected_group='+str(group)+' selected_local='+str(local)+' label='+label in audit,'Benchmark selected binding differs from original append')
    require(all(sha(p)==h for p,h in originals.items()) and all(sha(m['path'])==m['sha256'] for m in models) and sha(source)==copy_hash,'Original/disposable input changed')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()) and all(sha(e['path'])==e['sha256'] for e in executables),'Source/binary changed')
    require(sha(prior_editor)==editor_hash,'Editor changed')
    evidence=cache_evidence+ifm_evidence
    result=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),benchmark_cases=4,exact_output_bytes=len(outputs[0]),
        virtual_entries=sum(len(t['entries']) for t in tables),local_to_master_entries=sum(sum(map(len,t['maps'])) for t in tables),safety=safety.strip(),lookup_tables=lookup_tables,lookup_requests=len(lookup_expected),exact_lookup_bytes=len(lookup_outputs[0]),lookup_query_sha256=sha(lookup_query),
        benchmark_selection=selections,models=models,original_sha256=originals,sources_sha256=sources,executables=executables,warnings=warnings,
        query_sha256=sha(query),benchmark_query_sha256=sha(benchmark),static_instruction_bytes=sum(e['bytes'] for e in evidence),
        static_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in evidence],prior_editor=dict(path=str(prior_editor),sha256=editor_hash,rebuilt=False,run=False),
        limitations=[
            'Complete original datacache10013060 append executes with optional global name cache explicitly disabled, controlled group header services, preallocated sequence/map storage and a thread-ID import bridge. No DLL initialization or engine process.',
            'Original ASCII label comparison/override/append/mapping execute; allocation/container initialization and group loading are fixture services. Native name-cache construction/equivalence and recursive include/group dedup lifetime remain unverified.',
            'Actual original append tables feed complete original IFM105DA450 lookup in a separate private process; group header resolution is controlled. All table indices and signed/out-of-range requests match the owned lookup and independent table-index expectations.',
            'Actual benchmark root plus its single nonrecursive included model are read from mounted disk files; graph evaluated at two frames by owned evaluator. Sequence sync getter/clamp and include-call ordering have static byte evidence, not full native scene lifecycle execution.',
            'Selected sequence metadata is now known under this group-order contract. Actual complete child/bounds/draw-frame/illumination/ambient integration is still unfinished; no native active setting or pixel parity is claimed.',
            'No existing editor dependency changed; no editor rebuild/run, computer use, original process access, original writes, SFM launch/control/close, Blender or GPU job.',
        ])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',cases=len(cases),exact_output_bytes=len(outputs[0]),benchmark_selection=selections),indent=2));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
