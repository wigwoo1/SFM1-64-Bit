"""Complete original296B0 convex search vs owned AMD64 dependency composition."""
import collections,hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_physics_support import encode as encodeSupport,expected as supportExpected,tetra
from verify_physics_line_simplex import expected as lineExpected
from verify_physics_triangle_simplex import expected as triangleExpected
from verify_physics_tetrahedron_simplex import expected as tetraExpected
from verify_physics_refinement import expected as refinementExpected
from verify_physics_contact_plane import expected as planeExpected
from verify_physics_triangle_contact_time import expected as timingExpected
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-search-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def frame(data):return struct.pack('<I',len(data))+data
def take(data,at):
    require(at+4<=len(data),'Missing frame');n=struct.unpack_from('<I',data,at)[0];require(at+4+n<=len(data),'Truncated frame');return data[at+4:at+4+n],at+4+n
def read_input(data,at):
    start=at;require(at+92<=len(data),'Truncated search prefix');_,at=take(data,at+92);require(at+112<=len(data),'Truncated search state/tolerance');at+=112;return data[start:at],at
def expected(row):
    wire,stateAt=take(row,92);wire=bytearray(wire);mode=struct.unpack_from('<I',wire)[0];ray=struct.unpack_from('<16f',row);axes=struct.unpack_from('<6f',row,64);point=struct.unpack_from('<I',row,88)[0];trace=bytearray(row[stateAt:stateAt+92]);direction=struct.unpack_from('<3f',row,stateAt+92);distanceBytes=row[stateAt+104:stateAt+108];tolerance=struct.unpack_from('<f',row,stateAt+108)[0];simplex=bytearray(68);stats=[0]*6
    mul=lambda a,b:f32(a*b);add=lambda a,b:f32(a+b);sub=lambda a,b:f32(a-b);div=lambda a,b:f32(a/b)
    def dot(a,b):return add(add(mul(a[1],b[1]),mul(a[0],b[0])),mul(a[2],b[2]))
    def normalized():
        v=[-x for x in direction];length=f32(math.sqrt(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]))));scale=div(1,add(length,2**-23));return [mul(x,scale) for x in v]
    def finish(answer,code):
        stats[5]=code;return bytes(trace)+struct.pack('<3f',*direction)+distanceBytes+struct.pack('<I',answer)+(bytes(wire[136:2186]) if mode==0 else b''),tuple(stats)
    def clearTrace():
        trace[:]=bytes(92);struct.pack_into('<f',trace,44,1.)
    def sample(d):
        feature=0;a=[0.,0.,0.]
        if not point:
            for axis in range(3):
                side=struct.unpack('<I',struct.pack('<f',d[axis]))[0]>>31;a[axis]=axes[axis*2+side];feature|=side<<axis
        end=dot(ray[6:9],d)>0;endpoint=ray[3:6] if end else ray[:3];struct.pack_into('<3f',wire,108,*[-x for x in d]);b=supportExpected(wire)
        if mode==0:wire[136:2186]=b[16:]
        featureB,*p=struct.unpack_from('<I3f',b);return struct.pack('<3fHH',*[sub(add(a[j],endpoint[j]),p[j]) for j in range(3)],(feature&0x7fff)|(0x8000 if end else 0),featureB&0xffff)
    def reduce(incoming):
        nonlocal direction,simplex
        count=struct.unpack_from('<I',simplex,64)[0]
        if count==0:simplex[:16]=incoming;struct.pack_into('<I',simplex,64,1);direction=struct.unpack_from('<3f',incoming);return False
        if count>3:return True
        answer=[None,lineExpected,triangleExpected,tetraExpected][count](bytes(simplex)+incoming+struct.pack('<3f',*direction));simplex=bytearray(answer[:68]);direction=struct.unpack_from('<3f',answer,68);return bool(struct.unpack_from('<I',answer,80)[0])
    if all(x==0 for x in direction):direction=(1.,0.,0.)
    search=normalized()
    for outer in range(100):
        stats[0]+=1;incoming=sample(search);projection=dot(struct.unpack_from('<3f',incoming),search)
        if projection<0:
            best=abs(projection)
            if not tolerance>best:return finish(0,1)
            if not ray[12]>0:return finish(0,2)
            bestDirection=search[:]
            if best>0:
                for inner in range(20):
                    stats[1]+=1;previous=struct.unpack_from('<3f',incoming);reduce(incoming);search=normalized();incoming=sample(search);candidate=-dot(struct.unpack_from('<3f',incoming),search)
                    if candidate>tolerance:return finish(0,3)
                    if candidate>best:best=candidate;bestDirection=search[:]
                    old=-dot(previous,search)
                    if sub(candidate,old)>struct.unpack('<f',struct.pack('<I',0xb8d1b717))[0]:break
            motion=-((ray[7]*bestDirection[1]+ray[6]*bestDirection[0])+ray[8]*bestDirection[2]);rounded=f32(motion)
            if not -tolerance*.1>motion and not (struct.unpack('<f',struct.pack('<I',0xb8d1b717))[0]>rounded and tolerance*.9>best):return finish(0,4)
            clearTrace();distance=sub(ray[12],mul(mul(sub(tolerance,best),ray[13]),div(-1.,rounded)))
            if distance<0:
                distance=0.;backoff=0.
                if struct.unpack_from('<I',simplex,64)[0]==3:
                    stats[4]+=1;answer=timingExpected(bytes(simplex)+struct.pack('<4f',*ray[9:12],tolerance))[0];backoff=struct.unpack('<f',answer)[0]
                if ray[12]>backoff:distance=sub(ray[12],backoff)
            struct.pack_into('<3f',trace,24,*[-x for x in bestDirection]);struct.pack_into('<I',trace,48,1);distanceBytes=struct.pack('<f',distance);return finish(1,5)
        if struct.unpack_from('<I',simplex,64)[0]>3 or reduce(incoming):
            clearTrace()
            if ray[12]!=0:
                stats[2]+=1;ref=refinementExpected(bytes(simplex)+row[:92]+frame(wire))[0];simplex=bytearray(ref[:68]);time=struct.unpack_from('<f',ref,68)[0]
                if mode==0:wire[136:2186]=ref[72:]
                if ray[12]>time and time>0:
                    distance=sub(ray[12],time);stats[3]+=1;plane=planeExpected(bytes(simplex)+row[:92]+frame(wire)+bytes(trace[:60])+bytes(24))[0];trace[24:40]=plane[24:40];facing=dot(ray[9:12],struct.unpack_from('<3f',trace,24))
                    if facing<0:distance=add(distance,div(tolerance,facing))
                    if distance<0:distance=0.
                    struct.pack_into('<I',trace,48,1);distanceBytes=struct.pack('<f',distance);return finish(1,6)
            distanceBytes=bytes(4);trace[54:56]=bytes([1,1]);return finish(1,7)
        search=normalized()
    return finish(0,8)
def make_ray(start,delta):
    end=[f32(start[j]+delta[j]) for j in range(3)];length=f32(math.sqrt((delta[0]*delta[0]+delta[1]*delta[1])+delta[2]*delta[2]));inverse=f32(1/length) if length>0 else 0.;unit=[f32(x*inverse) for x in delta] if length>0 else delta[:];return list(start)+end+list(delta)+unit+[length,length,inverse,0.]
def make_row(mode,j,rng):
    transform=[1,0,0,0,0,0,1,0,0,-1,0,0]
    if mode==0:points,triangles=tetra()
    else:points=[[float(rng.randint(-4,4)) for _ in range(3)] for _ in range(8)];triangles=[]
    wire=encodeSupport(mode,points,(1,2,3),transform=transform,first=rng.randint(0,16) if mode==1 else 0,triangles=triangles,cache=[rng.randrange(65536) for _ in range(1024)]+[65535] if mode==0 else None)
    start=[f32(rng.randint(-32,32)/4) for _ in range(3)];delta=[f32(rng.randint(-16,16)/4) for _ in range(3)] if j%17 else [0.,0.,0.];ray=make_ray(start,delta);axes=[float(rng.randrange(3)) if k%2==0 else -float(rng.randrange(3)) for k in range(6)];point=j%2
    numeric=bytes(rng.getrandbits(8) for _ in range(60));trace=numeric+struct.pack('<QIIHHQI',0 if j%5==0 else rng.getrandbits(32)+1,rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(16),rng.getrandbits(16),rng.getrandbits(32),rng.getrandbits(32));direction=[float(rng.randint(-3,3)) for _ in range(3)] if j%5 else [0.,-0.,0.];distance=bytes(rng.getrandbits(8) for _ in range(4));tolerance=f32(rng.choice([0.,.03125,.1,1.,4.,16.]));return struct.pack('<16f6fI',*ray,*axes,point)+frame(wire)+trace+struct.pack('<3f',*direction)+distance+struct.pack('<f',tolerance)
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior trace status');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        source=Path(e['source']);require(sha(source)==e['sha256'],'Original library changed');shutil.copyfile(source,libraries/source.name)
    executables=[]
    for name,machine in [('search_native_x86',0x14c),('search_capture_x86',0x14c),('search_owned_x64',0x8664),('search_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');data=p.read_bytes();pe=struct.unpack_from('<I',data,60)[0];require(struct.unpack_from('<H',data,pe+4)[0]==machine and b'asInvoker' in data,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    captured=[];observed=[];observedTiming=[];models=[]
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior asset/query/trace changed');trace=case/'traces.bin';record=case/'captures.bin'
        p=subprocess.run([str(OUT/'search_capture_x86.exe'),str(libraries),m['phy'],str(queries),str(trace),str(record)],capture_output=True,text=True,timeout=45,cwd=case);(case/'capture.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Search capture failed '+str(case)+' '+p.stdout+p.stderr);require(trace.read_bytes()==old.read_bytes(),'Search observation changed original full trace');raw=record.read_bytes();at=0;calls=0
        while at<len(raw):
            require(at+4<=len(raw),'Missing query');query=struct.unpack_from('<I',raw,at)[0];at+=4;require(query<m['queries'],'Captured query index');row,at=read_input(raw,at);answer,at=take(raw,at);require(at+4<=len(raw),'Missing timing count');timing=struct.unpack_from('<I',raw,at)[0];at+=4;captured.append(row);observed.append(answer);observedTiming.append(timing);calls+=1
        models.append(dict(model=m['model'],path=str(case),calls=calls,captures_sha256=sha(record),traces_sha256=sha(trace)));print('PASS original',index+1,'search='+str(calls),flush=True)
    require(captured,'No real search calls');rng=random.Random(0x296b0);rows=[]
    for mode,amount in [(2,4096),(1,256),(0,256)]:
        for j in range(amount):rows.append(make_row(mode,j,rng))
    controlled=len(rows);rows+=captured;inputs=run/'queries.bin';inputs.write_bytes(b''.join(rows));reference=[expected(row) for row in rows];(run/'expected.bin').write_bytes(b''.join(frame(x[0]) for x in reference));outputs=[];metricOutputs=[];logs=[]
    for name in ['search_native_x86','search_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(inputs),str(dest)],capture_output=True,text=True,timeout=120);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Search kernel '+name+' '+logs[-1]);data=dest.read_bytes();at=0;parsed=[]
        while at<len(data):item,at=take(data,at);parsed.append(item)
        require(len(parsed)==len(rows),'Search output count');outputs.append(parsed);metrics=Path(str(dest)+'.stats').read_bytes();require(len(metrics)==len(rows)*24,'Search metric count');metricOutputs.append([struct.unpack_from('<6I',metrics,j*24) for j in range(len(rows))])
    differences=[]
    for j,row in enumerate(rows):
        a,b=outputs[0][j],outputs[1][j];c,stats=reference[j];actual=observed[j-controlled] if j>=controlled else a;time=observedTiming[j-controlled] if j>=controlled else metricOutputs[0][j][4]
        if not (a==b==c==actual and metricOutputs[0][j][4]==metricOutputs[1][j][4]==stats[4]==time and metricOutputs[1][j]==stats):
            differences.append(dict(case=j,captured=j>=controlled,input=row.hex(),native=a.hex(),owned=b.hex(),expected=c.hex(),observed=actual.hex(),native_stats=metricOutputs[0][j],owned_stats=metricOutputs[1][j],expected_stats=stats,observed_timing=time))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Search mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'search_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Search safety '+safety.stdout+safety.stderr)
    invalid=run/'invalid';invalid.mkdir();bad=[('empty',b'',True),('truncated_prefix',rows[0][:91],True),('truncated_state',rows[0][:-1],True)]
    for label,at,bits in [('bad_box_flag',88,2),('bad_mode',96,3)]:
        row=bytearray(rows[0]);struct.pack_into('<I',row,at,bits);bad.append((label,bytes(row),True))
    _,stateAt=take(rows[0],92)
    for label,at in [('nan_direction',stateAt+92),('nan_tolerance',stateAt+108)]:
        row=bytearray(rows[0]);struct.pack_into('<I',row,at,0x7fc00000);bad.append((label,bytes(row),True))
    invalidChecks=0
    for label,data,both in bad:
        path=invalid/(label+'.bin');path.write_bytes(data)
        for name in ['search_owned_x64']+(['search_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(path),str(invalid/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Invalid search fixture '+label+' '+name);invalidChecks+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_convex_search',0x100296b0,0x10029f52),('actual_search_call',0x1002a37c,0x1002a381),('complete_binding_constructor',0x100295f0,0x100296a6),('original_timing_call',0x10029da0,0x10029da5)])
    require(image.string(0x100c8050)=='**empty**','Original empty surface name');require([image.word(0x10029f54+j*4) for j in range(4)]==[0x100298e4,0x10029916,0x10029925,0x10029934],'Search outer table');require([image.word(0x10029f64+j*4) for j in range(4)]==[0x10029a1a,0x10029a4c,0x10029a5b,0x10029a6a],'Search inner table')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    sources=['source_physics_search_candidate.h','physics_search_test_fixture.h','native_physics_search_fixture.h','probe_physics_search.cpp','probe_physics_search_capture.cpp','test_physics_search.cpp','build_physics_search.cmd','verify_physics_search.py','source_physics_refinement_candidate.h','source_physics_refinement_face_candidate.h','source_physics_contact_plane_candidate.h','source_physics_line_simplex_candidate.h','source_physics_triangle_simplex_candidate.h','source_physics_tetrahedron_simplex_candidate.h','source_physics_triangle_contact_time_candidate.h','source_physics_support_candidate.h','physics_support_test_fixture.h','native_physics_refinement_fixture.h','native_physics_support_fixture.h','native_vphysics_library_fixture.h','verify_physics_support.py','verify_physics_line_simplex.py','verify_physics_triangle_simplex.py','verify_physics_tetrahedron_simplex.py','verify_physics_refinement.py','verify_physics_refinement_face.py','verify_physics_contact_plane.py','verify_physics_triangle_contact_time.py','verify_ifm_draw_packet.py']
    result=dict(status='PASS',cases=len(rows),controlled_cases=controlled,captured_real_calls=len(captured),differences=0,owned_exits=dict(collections.Counter(x[1][5] for x in reference)),actual_exits=dict(collections.Counter(x[1][5] for x in reference[controlled:])),native_timing_calls=sum(x[4] for x in metricOutputs[0]),actual_timing_calls=sum(observedTiming),max_outer=max(x[1][0] for x in reference),max_inner=max(x[1][1] for x in reference),safety=safety.stdout+safety.stderr,invalid_fixture_checks=invalidChecks,executables=executables,logs=logs,models=models,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),query_sha256=sha(inputs),static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in sources},limits=['Complete original296B0 and all dependencies execute intact; only full-call observation and29DA0 timing counter bridges installed in loaded workspace copies','Owned semantic trace has64bit opaque identifiers; test adapter normalizes empty surface pointer toid0 and other legacy pointer bit patterns to opaque identifiers, never dereferences them; default name/metadata verified in initialized original DLL','Normalized semantic trace/state/answer and2050 walk-cache bytes compared; unused native local-simplex stack bytes not promoted into owned state','Owned diagnostics agree independent replay; only timing-call diagnostic compared with native execution count; exit counters are owned/reference classification','Input prepared shape contexts/ray/state are reference-derived here. Whole PHY/cache lifetime/tree traversal/caller hit conversion/filter/runtime/editor integration remain incomplete','No original writes, editor rebuild/integration, rendering/animation/serialization changes, UI/Blender launch or dependency installation'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','controlled_cases','captured_real_calls','differences','owned_exits','actual_exits','native_timing_calls','actual_timing_calls','max_outer','max_inner','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
