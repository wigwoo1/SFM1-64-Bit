"""Intact original engine TraceRay policy vs owned semantics; world/partition/clip services controlled."""
import collections,hashlib,json,random,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/engine-point-trace-20261016'
ENGINE=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/engine.dll')
ENGINE_SHA='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def framed(b):return struct.pack('<I',len(b))+b
def parse_frame(raw,at):
    require(at+4<=len(raw),'Missing output frame');n=struct.unpack_from('<I',raw,at)[0];require(at+4+n<=len(raw),'Short output frame');return raw[at+4:at+4+n],at+4+n
def make_trace(rng,frac=None,start=None,allsolid=None,left=None):
    floats=[f(rng.randrange(-10000,10001)/16) for _ in range(10)];fraction=rng.choice([0.,.125,.25,.5,.75,1.]) if frac is None else frac;inside=rng.choice([0,0,0,0,1,2]) if start is None else start;solid=rng.choice([0,0,0,0,1,2]) if allsolid is None else allsolid;exit=rng.choice([0.,.125,.25,.5,.75,1.]) if left is None else left
    b=bytearray(struct.pack('<10f4sfIHBBfQIIHHQI',*floats,bytes(rng.getrandbits(8) for _ in range(4)),fraction,rng.getrandbits(32),rng.getrandbits(16),solid,inside,exit,rng.choice([0,rng.getrandbits(32)+1]),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(16),rng.getrandbits(16),rng.getrandbits(32),rng.getrandbits(32)));require(len(b)==92,'Trace schema');return bytes(b)
def encode(start,delta,swept,mask,mode,excluded,world,candidates):return struct.pack('<6f4I',*start,*delta,swept,mask,mode,excluded)+world+struct.pack('<I',len(candidates))+b''.join(struct.pack('<I',h)+t for h,t in candidates)
def decode(row):
    head=struct.unpack_from('<6f4I',row);n=struct.unpack_from('<I',row,132)[0];require(len(row)==136+96*n,'Fixture length');return head,row[40:132],[(struct.unpack_from('<I',row,136+i*96)[0],row[140+i*96:232+i*96]) for i in range(n)]
def expected(row,counters):
    head,world,candidates=decode(row);start,delta=head[:3],head[3:6];swept,mask,mode,excluded=head[6:];result=bytearray(world);calls=[];partition=0;overflows=0;short=[0.,0.,0.];counters['world_calls']+=1
    if result[55] or mode==1:counters['early_returns']+=1
    else:
        partition=1;scale=struct.unpack_from('<f',result,44)[0];left=struct.unpack_from('<f',result,56)[0];exitScale=scale
        if scale==0:exitScale=left;struct.pack_into('<f',result,56,1.);counters['zero_world_fraction']+=1
        else:
            short=[f(f(f(delta[j]*scale)+start[j])-start[j]) for j in range(3)];struct.pack_into('<f',result,56,f(left/scale))
            if any(short[j]!=f(delta[j]*scale) for j in range(3)):counters['rounded_shortening']+=1
        struct.pack_into('<f',result,44,1.);overflows=max(0,len(candidates)-1024);counters['overflowed_candidates']+=overflows
        for i,(handle,trace) in enumerate(candidates[:1024]):
            label=i+1;calls.extend([(1,label),(2,label),(2,label)]);static=handle&0xffff0000==0x40000000
            if not static or excluded==label:counters['filtered']+=1;continue
            calls.append((3,label));counters['clipped']+=1;fraction=struct.unpack_from('<f',result,44)[0];candidateFraction=struct.unpack_from('<f',trace,44)[0]
            if trace[54] or trace[55] or fraction>candidateFraction:
                old=bytes(result);result[:]=trace;counters['merged']+=1
                if old[55]:
                    result[55]=1;counters['preserved_inside']+=1;a,b=struct.unpack_from('<f',old,56)[0],struct.unpack_from('<f',trace,56)[0]
                    if a>b:result[56:60]=old[56:60];result[:12]=old[:12];counters['preserved_exit_start']+=1
                if result[54]:counters['allsolid_stops']+=1;break
        struct.pack_into('<f',result,44,f(scale*struct.unpack_from('<f',result,44)[0]));struct.pack_into('<f',result,56,f(exitScale*struct.unpack_from('<f',result,56)[0]))
    return bytes(result)+struct.pack('<3I3fI',1,partition,overflows,*short,len(calls))+b''.join(struct.pack('<2I',*x) for x in calls)
def main():
    require(sha(ENGINE)==ENGINE_SHA,'Original engine changed');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));rng=random.Random(0x16ece0);rows=[]
    # Every world fraction/filter/start-solid and selected candidate flag state.
    for mode in [1,3]:
        for worldFraction in [0.,.125,.5,1.]:
            for worldInside in [0,1,2]:
                for static in [False,True]:
                    for excluded in [0,1,2]:
                        for solid,inside in [(0,0),(0,1),(1,0),(1,1)]:
                            world=make_trace(rng,worldFraction,worldInside,0,.75);candidates=[((0x40000000 if static else 0x40010000)+j,make_trace(rng,.25,inside,solid,.5)) for j in [1,2]];rows.append(encode([1e7,-1e7,-0.],[.5,-.25,0.],1,0x4481,mode,excluded,world,candidates))
    directed=len(rows)
    for i in range(8192):
        n=rng.randrange(0,17);mode=rng.choice([1,3]);start=[f(rng.uniform(-1e6,1e6)) for _ in range(3)];delta=[f(rng.uniform(-1000,1000)) for _ in range(3)];world=make_trace(rng,start=0 if i%5 else None,allsolid=0);candidates=[]
        for j in range(n):candidates.append((rng.choice([0x40000000,0x40000000,0,0x40010000,0xffff0000])+rng.randrange(65536),make_trace(rng)))
        rows.append(encode(start,delta,i%2,rng.getrandbits(32),mode,rng.randrange(n+1),world,candidates))
    # Original collector retains first1024 and issues a warning per extra item.
    for n in [0,1,1023,1024,1025,2048]:
        world=make_trace(rng,1.,0,0,0.);candidates=[(0x40000000+j,make_trace(rng,1.,0,0,0.)) for j in range(n)];rows.append(encode([0,0,0],[1,2,3],1,1,3,0,world,candidates))
    inputPath=run/'inputs.bin';inputPath.write_bytes(b''.join(rows));coverage=collections.Counter();reference=[expected(row,coverage) for row in rows];(run/'expected.bin').write_bytes(b''.join(framed(x) for x in reference));executables=[];outputs=[]
    for name,machine in [('engine_point_native_x86',0x14c),('engine_point_owned_x64',0x8664)]:
        p=OUT/(name+'.exe');raw=p.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'Executable architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)));dest=run/(name+'.bin');r=subprocess.run([str(p),str(ENGINE),str(inputPath),str(dest)],capture_output=True,text=True,timeout=120);(run/(name+'.log')).write_text(r.stdout+r.stderr);require(r.returncode==0,'Engine point probe '+name+' '+r.stdout+r.stderr);data=dest.read_bytes();at=0;parsed=[]
        while at<len(data):value,at=parse_frame(data,at);parsed.append(value)
        require(len(parsed)==len(rows),'Output count');outputs.append(parsed)
    safetyPath=OUT/'engine_point_safety_x64.exe';raw=safetyPath.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and b'asInvoker' in raw,'Safety architecture/manifest');executables.append(dict(path=str(safetyPath),sha256=sha(safetyPath),machine='0x8664'));safety=subprocess.run([str(safetyPath)],capture_output=True,text=True,timeout=15);(run/'safety.log').write_text(safety.stdout+safety.stderr);require(safety.returncode==0,'Engine point safety '+safety.stdout+safety.stderr)
    differences=[]
    for i,want in enumerate(reference):
        if outputs[0][i]!=want or outputs[1][i]!=want:
            differences.append(dict(case=i,input=rows[i].hex(),native=outputs[0][i].hex(),owned=outputs[1][i].hex(),expected=want.hex()))
            if len(differences)==4:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Engine point mismatch '+str(run/'comparison.json'))
    invalid=run/'invalid';invalid.mkdir();bad=[('empty',b''),('truncated',rows[0][:-1])]
    for label,offset,value in [('bad_mode',32,2),('bad_swept',24,2),('bad_count',132,2049),('bad_excluded',36,10000),('nan_start',0,0x7fc12345),('nan_world_fraction',84,0x7fc12345)]:
        data=bytearray(rows[0]);struct.pack_into('<I',data,offset,value);bad.append((label,bytes(data)))
    checks=0
    for label,data in bad:
        path=invalid/(label+'.bin');path.write_bytes(data)
        for name in ['engine_point_native_x86','engine_point_owned_x64']:
            p=subprocess.run([str(OUT/(name+'.exe')),str(ENGINE),str(path),str(invalid/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Invalid fixture '+label+' '+name);checks+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(ENGINE,'engine-disassembly.txt',[('complete_engine_trace_ray',0x1016ece0,0x1016f0d3),('complete_merge',0x1016ebd0,0x1016ece0),('complete_base_trace_copy',0x10146de0,0x10146e63),('complete_candidate_collector',0x1016d8f0,0x1016d923),('complete_trace_initialize',0x10145f40,0x10145f7d),('complete_partition_getter',0x101bea40,0x101bea46)])
    require(image.word(0x10354478)==0x1016d8f0,'Original collector virtual');require(sha(ENGINE)==ENGINE_SHA,'Original engine changed after tests');sources=['source_engine_point_trace_candidate.h','engine_point_trace_test_fixture.h','probe_engine_point_trace.cpp','build_engine_point_trace.cmd','verify_engine_point_trace.py','test_engine_point_trace.cpp','source_world_light_entity_filter_candidate.h','source_physics_search_candidate.h','physics_search_test_fixture.h','native_private_world_image_fixture.h','native_physics_search_fixture.h']
    result=dict(status='PASS',cases=len(rows),directed_cases=directed,safety=safety.stdout.strip(),differences=0,coverage=dict(coverage),invalid_fixture_checks=checks,executables=executables,engine_sha256=ENGINE_SHA,input_sha256=sha(inputPath),static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in sources},limits=['Original complete1016ECE0 and merge/copy/collector/filter/manager helpers execute intact in private engine image; no DllMain/import resolution','World trace call redirected to controlled host; world annotation, spatial partition enumeration, entity handle/collideable resolution and ClipRayToCollideable virtual are controlled services','Controlled clipping returns fixture semantic traces, NOT real PHY collision results; original physics dispatch/shape binding and spatial registry not verified here','Owned entities are native-width tokens deliberately sharing low32 bits; native x86 identities normalize to fixture labels in event wire','Source engine result includes exact92 semantic bytes plus world/partition/overflow counters, shortened delta and ordered resolve/handle/clip calls','Existing BSP lighting guard remains; no editor build/integration, original writes, GUI/Blender/dependencies or whole-project completion claim'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','directed_cases','differences','coverage','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
