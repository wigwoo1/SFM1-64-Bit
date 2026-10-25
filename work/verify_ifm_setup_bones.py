"""Compare complete original SetupBones wrapper/preparation offline."""
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM, IFM_SHA, SESSION, SESSION_SHA

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT/'outputs/setup-bones-20261025'
IDENTITY = [1,0,0,0,0,1,0,0,0,0,1,0]
UNCHANGED = struct.pack('<12f', *([-9876.5]*12))*256


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise RuntimeError(message)


def encode(mode=1, flags=127, count=3, locals=None, cache_count=None, world=IDENTITY,
           scale=1, typed=None, flex=(.1,.2), retained_flex=(.9,.8,.7), tick=50000, requests=None):
    if locals is None: locals=count
    if cache_count is None: cache_count=count
    if typed is None: typed=[7]*count
    if requests is None:
        requests=[(256,0x100,0x40a00000,1),(256,0,0x7fc00001,1),
                  (cache_count-1,0xffffffff,0,1),(-2147483648,0,0xff800000,0)]
    poses=b''.join(struct.pack('<iI8f',i-1,0x100,1+i,2,3,0,0,0,1,[1,.5,2][i%3]) for i in range(count))
    cache=b''.join(struct.pack('<12f',*(IDENTITY[:3]+[float(i)]+IDENTITY[4:])) for i in range(256))
    return struct.pack('<9I12ff',mode,flags,cache_count,count,locals,len(requests),len(flex),len(retained_flex),tick&0xffffffff,*world,scale)+poses+struct.pack('<'+str(count)+'I',*typed)+struct.pack('<'+str(len(flex))+'f',*flex)+struct.pack('<'+str(len(retained_flex))+'f',*retained_flex)+cache+b''.join(struct.pack('<i3I',*r) for r in requests)


def decode_queries(raw):
    at=0;rows=[]
    while at<len(raw):
        h=struct.unpack_from('<9I',raw,at);world=struct.unpack_from('<12f',raw,at+36);scale=struct.unpack_from('<f',raw,at+84)[0];cursor=at+88
        poses=raw[cursor:cursor+h[3]*40];cursor+=h[3]*40
        typed=struct.unpack_from('<'+str(h[3])+'I',raw,cursor);cursor+=h[3]*4
        flex=raw[cursor:cursor+h[6]*4];cursor+=h[6]*4
        retained=raw[cursor:cursor+h[7]*4];cursor+=h[7]*4
        cache=raw[cursor:cursor+256*48];cursor+=256*48
        requests=[struct.unpack_from('<i3I',raw,cursor+i*16) for i in range(h[5])];cursor+=h[5]*16
        require(cursor<=len(raw),'Query truncated')
        rows.append(dict(header=h,world=world,scale=scale,poses=poses,typed=typed,flex=flex,retained=retained,cache=cache,requests=requests));at=cursor
    require(at==len(raw),'Query framing');return rows


def decode_output(raw,rows):
    at=0;result=[]
    for row in rows:
        steps=[]
        for request in row['requests']:
            success,n,flex_n=struct.unpack_from('<3I',raw,at);at+=12
            trace=struct.unpack_from('<'+str(n)+'I',raw,at);at+=n*4
            frame=raw[at:at+24];at+=24
            flex=raw[at:at+flex_n*4];at+=flex_n*4
            cache=raw[at:at+256*48];at+=256*48
            output=raw[at:at+256*48];at+=256*48
            require(len(output)==256*48,'Output truncated')
            steps.append(dict(success=success,trace=trace,frame=frame,flex=flex,cache=cache,output=output))
        result.append(steps)
    require(at==len(raw),'Output framing');return result


def main():
    run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
    game=IFM.parent.parent.parent
    originals={str(p):sha(p) for p in [IFM,SESSION,game/'usermod/gameinfo.txt']}
    require(originals[str(IFM)]==IFM_SHA and originals[str(SESSION)]==SESSION_SHA
            and originals[str(game/'usermod/gameinfo.txt')]=='ef7f43f76306e6cf2852257d3f9ef068a4fa498343a935c54d91d0a9912247de','Original pins')
    sources={p:sha(ROOT/'work'/p) for p in ['ifm_setup_bones_candidate.h','probe_ifm_setup_bones.cpp',
        'export_benchmark_setup_bones.cpp','test_ifm_setup_bones.cpp','build_ifm_setup_bones.cmd','verify_ifm_setup_bones.py',
        'ifm_bone_world_palette_candidate.h','ifm_draw_frames_candidate.h','session_ifm_bone_inputs_candidate.h',
        'session_ifm_bone_references_candidate.h','probe_vulkan_win32_surface_x64.cpp']}
    editor=ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe';editor_sha=sha(editor)
    require(editor_sha=='2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2','Prior editor')
    image,evidence=check_ranges(IFM,'ifm-disassembly.txt',[
        ('complete_public_SetupBones_wrapper',0x1023ce60,0x1023cf1d),
        ('complete_primary_preparation',0x10241820,0x10241f4f),
        ('complete_normal_and_precomputed_palette_kernel',0x102411b0,0x1024181f),
        ('complete_root_frame_update',0x1023e0b0,0x1023e136),
        ('constructor_primary_and_renderable_vtables',0x1024249f,0x102424ac),
        ('model_flex_array_accessor',0x101defa0,0x101defa7),
    ])
    require(image.word(0x10adb938)==0x1023ce60 and image.word(0x10adb9b8+12*4)==0x10241820,'Actual constructor/vtable setup dispatch')
    executables=[];warnings={}
    for tag,name,machine in [('native','setup_native_x86',0x14c),('owned','setup_owned_x64',0x8664),
                             ('export','setup_benchmark_x64',0x8664),('safety','setup_safety_x64',0x8664)]:
        log=(OUT/(tag+'-build.log')).read_text();warnings[tag]=[x for x in log.splitlines() if 'warning ' in x.lower()]
        require('error C' not in log and all(tag=='export' and 'C4458' in x and 'graph_transform_hierarchy.h' in x for x in warnings[tag]),'Build diagnostics '+tag)
        exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine
                and struct.unpack_from('<H',raw,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in raw,'Architecture/non-elevated manifest '+name)
        executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))

    def execute(name,args,label=None):
        p=subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (run/((label or name)+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode==0,name+' exit='+str(p.returncode)+' '+p.stdout+p.stderr);return p.stdout

    safety=execute('setup_safety_x64',[]);require('checks=9 refusals=4' in safety,'Safety coverage')
    cases=[]
    # Public wrapper: callback false/true, mutation on failure, clock, null output,
    # negative/extreme capacity and ignored public time, repeated on same object.
    for clock in [0,8]:
        for result in [0,256]:
            for mutate in [0,512]:
                for cache_count in [0,1,3,65,256]:
                    cases.append(encode(mode=0,flags=clock|result|mutate,count=0,cache_count=cache_count))
    wrapper_cases=len(cases)
    for flags in range(256):
        for locals in [0,2]:
            cases.append(encode(flags=flags,locals=locals,typed=[flags%8,(flags//8)%8,7],scale=2))
    rng=random.Random(2026102505)
    for i in range(128):
        n=rng.randrange(1,33);world=IDENTITY.copy();scale=rng.choice([.5,1,2,-1]);world=[v*scale if j%4!=3 else v for j,v in enumerate(world)]
        world[3],world[7],world[11]=[rng.uniform(-5000,5000) for _ in range(3)]
        cases.append(encode(count=n,locals=rng.randrange(n+1),flags=rng.randrange(256),world=world,
            scale=rng.choice([.25,1,2,-2]),typed=[rng.randrange(8) for _ in range(n)],
            flex=[rng.uniform(-2,2) for _ in range(i%17)],retained_flex=[rng.uniform(-2,2) for _ in range(i%23)],
            tick=rng.choice([-2147483648,-1,0,50000,50417,2147483647])))
    cases.append(encode(count=256,flags=127,flex=list(range(256)),retained_flex=()))
    query=run/'queries.bin';query.write_bytes(b''.join(cases));rows=decode_queries(query.read_bytes())
    output=[]
    for name in ['setup_native_x86','setup_owned_x64']:
        target=run/(name+'.bin');execute(name,[IFM,query,target]);output.append(target.read_bytes())
    require(output[0]==output[1],'Full wrapper/preparation differs; retain first mismatch')
    decoded=decode_output(output[0],rows)
    for row,steps in zip(rows,decoded):
        for request,record in zip(row['requests'],steps):
            trace=record['trace'];require(trace[0]==1 and trace[-1]==9 and trace.count(3)==1,'Every public call must prepare exactly once and end profiling')
            if not record['success'] or not request[3]:require(record['output']==UNCHANGED,'Failed/null output changed output storage')
            else:
                n=row['header'][2]*48
                require(record['output'][:n]==record['cache'][:n] and record['output'][n:]==UNCHANGED[n:],'Successful cache copy extent')
        if row['header'][0]==0:
            expected=bool(row['header'][1]&256)
            require([bool(s['success']) for s in steps]==[expected,expected,False,expected],'Independent wrapper capacity/null gate')
            if row['header'][1]&512:
                require(struct.unpack_from('<f',steps[-1]['cache'],12)[0]==4,'Preparation mutation retained on every repeated call')
    prior_dir=ROOT/'outputs/benchmark-draw-bounds-20261025/verification-czdinbt0'
    source=prior_dir/'benchmark-copy.dmx';prior=json.loads((prior_dir/'results.json').read_text());require(sha(source)==SESSION_SHA,'Disposable benchmark copy provenance')
    benchmark=run/'benchmark-queries.bin';audit=execute('setup_benchmark_x64',[source,game/'usermod/gameinfo.txt',benchmark])
    require('PASS setup_benchmark_cases=4' in audit and audit.count('drivers=0 precomputed=0')==4,'Actual benchmark export')
    models=[]
    for path in sorted(set(re.findall(r'^model_path=(.+)$',audit,re.M))):
        digest=sha(path);require(any(m['path']==path and m['sha256']==digest for m in prior['models']),'Current mounted MDL pin')
        b=Path(path).read_bytes();h2=struct.unpack_from('<I',b,0x190)[0];require(not h2 or struct.unpack_from('<I',b,h2+24)[0]==0,'Native bone-flex driver count')
        models.append(dict(path=path,sha256=digest));originals[path]=digest
    require(len(models)==2,'Two actor models')
    benchmark_rows=decode_queries(benchmark.read_bytes());require([r['header'][3] for r in benchmark_rows]==[65,3,65,3],'Benchmark bone counts')
    benchmark_out=[]
    for name in ['setup_native_x86','setup_owned_x64']:
        target=run/('benchmark-'+name+'.bin');execute(name,[IFM,benchmark,target],'benchmark-'+name);benchmark_out.append(target.read_bytes())
    require(benchmark_out[0]==benchmark_out[1],'Actual full benchmark preparation differs')
    bench_decoded=decode_output(benchmark_out[0],benchmark_rows);palette=b''
    for row,steps in zip(benchmark_rows,bench_decoded):
        require([s['success'] for s in steps]==[1,1,0,1],'Benchmark return/copy failure behavior')
        require(all(s['trace']==(1,2,3,4,5,2,7,6)+((8,) if i==2 else ())+(9,) for i,s in enumerate(steps)),'Complete benchmark call order')
        require(all(s['cache']==steps[0]['cache'] and s['flex']==row['flex'] for s in steps),'Public time/mask affected fixed normal preparation')
        palette+=steps[0]['cache'][:row['header'][3]*48]
    # Independently retained original normal results, already proved against
    # typed prepass snapshots in the immediately preceding milestone.
    previous=ROOT/'outputs/precomputed-bone-palette-20261025/verification-50y9kg_7'
    previous_receipt=json.loads((previous/'results.json').read_text())
    baseline=previous/'benchmark-normal_native_x86.bin'
    require(len(palette)==previous_receipt['normal_palette_bytes']==6528 and palette==baseline.read_bytes(),'Complete preparation differs from retained verified normal palettes')
    require(all(sha(p)==h for p,h in originals.items()) and sha(source)==SESSION_SHA,'Original/disposable input changed')
    require(all(sha(ROOT/'work'/p)==h for p,h in sources.items()) and all(sha(e['path'])==e['sha256'] for e in executables) and sha(editor)==editor_sha,'Source/executable changed')
    result=dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),requests=sum(len(r['requests']) for r in rows),
        wrapper_cases=wrapper_cases,prepared_cases=len(rows)-wrapper_cases,exact_output_bytes=len(output[0]),
        benchmark_cases=4,benchmark_requests=16,exact_benchmark_bytes=len(benchmark_out[0]),exact_normal_palette_bytes=len(palette),
        safety=safety.strip(),warnings=warnings,executables=executables,sources_sha256=sources,original_sha256=originals,models=models,
        query_sha256=sha(query),benchmark_query_sha256=sha(benchmark),retained_palette_sha256=sha(baseline),
        static_instruction_bytes=sum(e['bytes'] for e in evidence),static_ranges=[{k:v for k,v in e.items() if k!='instructions'} for e in evidence],
        prior_editor=dict(path=str(editor),sha256=editor_sha,rebuilt=False,run=False),limitations=[
            'Complete original public1023CE60 and primary10241820 plus original kernel/decomposition/Euler helpers execute in a private PE image. Profiling/clock/MDL lock/header load/sync/DM type/handle/scale/world/CRT services are controlled. No native engine model/cache/container initialization or concurrency.',
            'Primary preparation is not a same-time cache shortcut: every wrapper request invokes it with INT_MIN, and supplied public time is ignored. Output capacity is checked AFTER preparation; false copy result may retain prepared internal palettes/flex data.',
            'Fixtures preallocate256 bone/flex slots, bound local count to MDL count, exclude procedural-flag4 bones and bone-flex drivers, and provide no eligible precomputed DAG reference. The benchmark prepass zero-write condition was independently established previously and checked again during export.',
            'Benchmark local/root/flex inputs are freshly evaluated by the owned evaluator from an immutable disposable session copy and pinned mounted models. Raw-world matrices are supplied at the native DAG world accessor. No actual original animation evaluator or live runtime rendering flags measured.',
            'No editor dependency changed; no editor build/run, visible lighting/pixel parity claim, computer use, original process access/launch/control/close, original writes, Blender, GPU job, dependency install or publication.',
        ])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status','cases','requests','wrapper_cases','prepared_cases','exact_output_bytes','benchmark_requests','exact_normal_palette_bytes']},indent=2));print('receipt='+str(run/'results.json'))


if __name__=='__main__':main()
