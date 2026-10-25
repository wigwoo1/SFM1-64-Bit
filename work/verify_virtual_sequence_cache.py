"""Original native name-cache construction/append vs explicit owned modes."""
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
from verify_benchmark_draw_binding import encode,decode_output,metadata
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM,IFM_SHA,SESSION,SESSION_SHA
from verify_studio_vcollide_ready import DATACACHE,DATACACHE_SHA
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/virtual-sequence-cache-20261025'
GAME=IFM.parent.parent.parent
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def require(v,m):
    if not v:raise RuntimeError(m)
def decode_queries(raw):
    at=0;cases=[]
    while at<len(raw):
        count=struct.unpack_from('<I',raw,at)[0];at+=4;groups=[]
        for _ in range(count):
            n=struct.unpack_from('<I',raw,at)[0];at+=4;group=[]
            for _ in range(n):
                flags,activity,length=struct.unpack_from('<III',raw,at);at+=12;name=raw[at:at+length].decode('ascii');at+=length;group.append((name,flags,activity))
            groups.append(group)
        cases.append(groups)
    require(at==len(raw),'Query framing');return cases
def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    originals={str(p):sha(p) for p in [IFM,DATACACHE,SESSION,GAME/'usermod/gameinfo.txt']}
    require(originals[str(IFM)]==IFM_SHA and originals[str(DATACACHE)]==DATACACHE_SHA and originals[str(SESSION)]==SESSION_SHA,'Original pins')
    tracked=['source_sequence_name_cache_candidate.h','source_mdl_virtual_sequences_candidate.h','probe_virtual_sequence_append.cpp','probe_virtual_sequence_lookup.cpp','test_virtual_sequence_append.cpp','audit_benchmark_draw_binding.cpp','build_virtual_sequence_cache.cmd','verify_virtual_sequence_cache.py','verify_benchmark_draw_binding.py','ifm_renderable_bounds_candidate.h','ifm_draw_frames_candidate.h','probe_vulkan_win32_surface_x64.cpp']
    sources={p:sha(ROOT/'work'/p) for p in tracked}
    prior_editor=ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe';editor_sha=sha(prior_editor)
    require(editor_sha=='2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2','Prior editor')
    _,evidence=check_ranges(DATACACHE,'datacache-disassembly.txt',[
        ('cache_entry_constructor',0x10012f30,0x10012fa2),('cache_label_copy_and_insert',0x10012fb0,0x10013056),
        ('complete_sequence_append',0x10013060,0x100134bf),('tree_less_comparator',0x10004d50,0x10004d80),
        ('tree_insert_search_and_node_allocation',0x10010f80,0x10011180),('tree_lookup',0x100116f0,0x100117f5),
        ('tree_rebalance',0x10011800,0x10011eab),('tree_link_and_rebalance_call',0x10011f60,0x10011fd3),
        ('ASCII_comparator',0x100185b0,0x1001862e),('cache_selection_gate',0x10013b2c,0x10013b5c),
    ])
    _,lookup_evidence=check_ranges(IFM,'ifm-disassembly.txt',[('complete_virtual_sequence_lookup',0x105da450,0x105da4e4)])
    builds=[('cached','sequence_cached_native_x86',0x14c),('uncached','sequence_uncached_native_x86',0x14c),('owned-cached','sequence_cached_owned_x64',0x8664),('owned','sequence_owned_x64',0x8664),('lookup-native','sequence_lookup_native_x86',0x14c),('lookup-owned','sequence_lookup_owned_x64',0x8664),('safety','sequence_safety_x64',0x8664),('audit','benchmark_draw_binding_x64',0x8664)]
    warnings={};executables=[]
    for tag,name,machine in builds:
        log=(OUT/(tag+'-build.log')).read_text();warnings[tag]=[x for x in log.splitlines() if 'warning ' in x.lower()]
        require('error C' not in log and all(tag=='audit' and 'C4458' in x and 'graph_transform_hierarchy.h' in x for x in warnings[tag]),'Build diagnostics '+tag)
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b),'Executable architecture '+name)
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    def execute(name,args,label):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=90)
        (run/(label+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,label+' '+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout
    safety=execute('sequence_safety_x64',[],'safety');require('checks=7 refusals=7' in safety,'Safety coverage')
    source=run/'benchmark-copy.dmx';shutil.copyfile(SESSION,source);copy_sha=sha(source);benchmark=run/'benchmark-queries.bin'
    audit=execute('benchmark_draw_binding_x64',[source,GAME/'usermod/gameinfo.txt',benchmark],'benchmark-audit');require('PASS benchmark_cases=4' in audit,'Fresh benchmark coverage')
    old=ROOT/'outputs/benchmark-draw-binding-20261025/verification-itwn42q0';old_receipt=json.loads((old/'results.json').read_text())
    require(sha(old/'queries.bin')==old_receipt['query_sha256'] and sha(benchmark)==old_receipt['benchmark_query_sha256'],'Retained corpus/fresh benchmark provenance')
    retained=decode_queries((old/'queries.bin').read_bytes());require(len(retained)==1031,'Retained case count')
    benchmark_cases=decode_queries(benchmark.read_bytes());models=[];raw_sequences={};raw_bounds={}
    pins={'450f8b480e0b85ed79b8747dae79c6d537cde1342bff4e77e5216d3008eabdd1','cf688be14884e63535e313e9a894e75a928f037f604f5193c426dcec9ae1eae6','3f5bb351433ecc1339476f28d840d8b0000281f8792be80179ac122f5ef77963'}
    for name in sorted(set(re.findall(r'^metadata_path=(.+)$',audit,re.M))):
        path=Path(name);seq,bounds=metadata(path);digest=sha(path);require(digest in pins,'Mounted MDL pin');models.append(dict(path=name,sha256=digest,local_sequences=len(seq)));raw_sequences[name]=seq;raw_bounds[name]=bounds
    require(len(models)==3,'Mounted model coverage')
    pyro=next(p for p in raw_sequences if p.replace('\\','/').endswith('/hwm/pyro.mdl'));included=next(p for p in raw_sequences if p.replace('\\','/').endswith('/pyro_animations.mdl'));flame=next(p for p in raw_sequences if p.replace('\\','/').endswith('/w_flamethrower.mdl'))
    require(benchmark.read_bytes()==b''.join(encode(g) for g in [[raw_sequences[pyro],raw_sequences[included]],[raw_sequences[flame]]]*2),'Independent raw metadata wire')
    cases=list(retained);rng=random.Random(2026102503);unique_indices=[]
    for _ in range(512):
        groups=[]
        for _ in range(rng.randrange(1,6)):
            names=rng.sample(range(80),rng.randrange(0,33));groups.append([(('seq' if rng.randrange(2) else 'SEQ')+str(n),rng.choice([0,0x800,0x801,0xffffffff]),rng.getrandbits(32)) for n in names])
        unique_indices.append(len(cases));cases.append(groups)
    directed_index=len(cases);cases.append([[('a',0x800,1),('A',0x800,2),('a',0x800,3)],[('A',4,7)]])
    cases.append([[('label'+str(i),i,4096-i) for i in range(4096)]])
    cases.append([[('equal',0x800,i) for i in range(2048)],[('EQUAL',1,9)]])
    query=run/'queries.bin';query.write_bytes(b''.join(map(encode,cases)))
    outputs={};tables={};logs={}
    for name in ['sequence_cached_native_x86','sequence_uncached_native_x86','sequence_cached_owned_x64','sequence_owned_x64']:
        target=run/(name+'.bin');logs[name]=execute(name,[DATACACHE,query,target],name);outputs[name]=target.read_bytes();tables[name]=decode_output(outputs[name],cases)
    require(outputs['sequence_cached_native_x86']==outputs['sequence_cached_owned_x64'],'Owned cached sequence policy differs from original')
    require(outputs['sequence_uncached_native_x86']==outputs['sequence_owned_x64'],'Uncached sequence regression')
    require('native_cache=constructed' in logs['sequence_cached_native_x86'] and 'remaining_allocations=0' in logs['sequence_cached_native_x86'],'Native cache/allocator lifecycle evidence')
    allocs=int(re.search(r' allocations=(\d+)',logs['sequence_cached_native_x86'])[1]);require(allocs>0,'Native original did not allocate cache')
    cached=tables['sequence_cached_native_x86'];uncached=tables['sequence_uncached_native_x86'];differences=[i for i,(a,b) in enumerate(zip(cached,uncached)) if a!=b]
    require(sum(i<len(retained) for i in differences)==703,'Retained duplicate-label distinction changed')
    require(not set(differences).intersection(unique_indices),'Unique-per-group equivalence failure')
    require(cached[directed_index]['maps']==[[0,1,2],[1]] and uncached[directed_index]['maps']==[[0,1,2],[0]],'Independent three-equal-label rotation expectation')
    benchmark_start=len(retained)-4;benchmark_selection=[]
    for i in range(4):
        idx=benchmark_start+i;require(cached[idx]==uncached[idx],'Actual benchmark changed with cache mode');sequence=57 if i%2==0 else 0;flags,activity,group,local=cached[idx]['entries'][sequence];paths=[pyro,included] if i%2==0 else [flame];path=paths[group]
        benchmark_selection.append(dict(tick=[50000,50000,50417,50417][i],model=13207 if i%2==0 else 13817,sequence=sequence,group=group,local=local,label=raw_sequences[path][local][0],bounds=raw_bounds[path][local],path=path))
    # Actual CACHE-ENABLED original table output, not an owned table, crosses
    # to a separate original IFM lookup process. Signed invalid requests go0.
    lookup_rows=[];expected=[];nonempty=0
    for groups,table in zip(cases,cached):
        n=len(table['entries'])
        if not n:continue
        requests=[-2147483648,-1,0,1,57,n-1,n,n+1,2147483647]+list(range(n))
        lookup_rows.append(struct.pack('<I',len(groups))+struct.pack('<'+str(len(groups))+'I',*map(len,groups))+struct.pack('<I',n)+b''.join(struct.pack('<4I',*e) for e in table['entries'])+struct.pack('<I',len(requests))+struct.pack('<'+str(len(requests))+'i',*requests))
        for index in requests:expected.append(struct.pack('<2I',*table['entries'][index if 0<=index<n else 0][2:]))
        nonempty+=1
    lookup=run/'lookup-queries.bin';lookup.write_bytes(b''.join(lookup_rows));lookup_outputs=[]
    for name in ['sequence_lookup_native_x86','sequence_lookup_owned_x64']:
        target=run/(name+'.bin');execute(name,[IFM,lookup,target],name);lookup_outputs.append(target.read_bytes())
    require(lookup_outputs[0]==lookup_outputs[1]==b''.join(expected),'Original cached append -> original/owned descriptor lookup differs')
    require(all(sha(p)==h for p,h in originals.items()) and all(sha(m['path'])==m['sha256'] for m in models) and sha(source)==copy_sha,'Original/fixture input changed')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()) and all(sha(e['path'])==e['sha256'] for e in executables) and sha(prior_editor)==editor_sha,'Source/binary/editor changed')
    result=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),cases=len(cases),cached_exact_bytes=len(outputs['sequence_cached_native_x86']),uncached_exact_bytes=len(outputs['sequence_uncached_native_x86']),mode_difference_cases=len(differences),retained_mode_differences=703,unique_per_group_cases=len(unique_indices),directed_duplicate_case=directed_index,
        cached_virtual_entries=sum(len(t['entries']) for t in cached),cached_local_maps=sum(sum(map(len,t['maps'])) for t in cached),native_allocation_calls=allocs,safety=safety.strip(),
        benchmark_selection=benchmark_selection,lookup_tables=nonempty,lookup_requests=len(expected),exact_lookup_bytes=len(lookup_outputs[0]),query_sha256=sha(query),lookup_query_sha256=sha(lookup),
        models=models,original_sha256=originals,sources_sha256=sources,executables=executables,warnings=warnings,static_instruction_bytes=sum(e['bytes'] for e in evidence+lookup_evidence),static_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in evidence+lookup_evidence],prior_editor=dict(path=str(prior_editor),sha256=editor_sha,rebuilt=False,run=False),
        limitations=[
            'Original cache constructor, name-copy/insert/search/rotations and complete append execute privately with native cache index0 and host allocator ABI. Temporary sequence/map initialization and group headers remain controlled.',
            'Owned explicit Enabled/Disabled modes reproduce the tested table and local-map values. Duplicate labels can change cached selection because rotations affect the first equal tree node; modes are not generally interchangeable.',
            'Fresh benchmark two-frame inputs match in both modes and feed separate original descriptor lookup. Full include/group construction, nested cache context push/pop, native cache destruction/concurrency and live model/cache state are outside this test.',
            'Actual child/bounds/draw-frame/illumination integration and rendered pixels remain unfinished. No editor dependency changed or complete editor rebuilt/run.',
            'No computer use, original process probes/access, original file writes, SFM launch/control/close, Blender, GPU job or dependency installation.',
        ])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','cached_exact_bytes','uncached_exact_bytes','mode_difference_cases','lookup_requests','exact_lookup_bytes','native_allocation_calls']},indent=2));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
