"""Complete original28F40 loop vs owned support/face/normalization composition."""
import collections,hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_physics_support import encode as encodeSupport,expected as supportExpected,tetra
from verify_physics_refinement_face import expected as faceExpected
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-refinement-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def frame(data):return struct.pack('<I',len(data))+data
def take(data,at):
    require(at+4<=len(data),'Missing frame');n=struct.unpack_from('<I',data,at)[0];require(at+4+n<=len(data),'Truncated frame');return data[at+4:at+4+n],at+4+n
def read_input(data,at):
    start=at;require(at+160<=len(data),'Truncated refinement prefix');at+=160;_,at=take(data,at);return data[start:at],at
def encode(points,ray,box,point,support,rng,count=4):
    state=bytearray(rng.getrandbits(8) for _ in range(68))
    for j,p in enumerate(points):struct.pack_into('<3f',state,j*16,*p)
    struct.pack_into('<I',state,64,count)
    return bytes(state)+struct.pack('<16f6fI',*ray,*box,point)+frame(support)
def expected(row):
    state=bytearray(row[:68]);wire,_=take(row,160);wire=bytearray(wire);mode=struct.unpack_from('<I',wire)[0]
    def result(value,iterations,outcome):return bytes(state)+struct.pack('<f',value)+(bytes(wire[136:2186]) if mode==0 else b''),iterations,outcome
    if struct.unpack_from('<i',state,64)[0]<4:return result(0,0,'early')
    ray=struct.unpack_from('<16f',row,68);axes=struct.unpack_from('<6f',row,132);point=struct.unpack_from('<I',row,156)[0];unit=ray[9:12]
    mul=lambda a,b:f32(a*b);add=lambda a,b:f32(a+b);sub=lambda a,b:f32(a-b)
    def dot(a,b):return add(add(mul(a[1],b[1]),mul(a[0],b[0])),mul(a[2],b[2]))
    def offset(a,b):return [sub(a[j],b[j]) for j in range(3)]
    def cross(a,b):return [sub(mul(a[1],b[2]),mul(a[2],b[1])),sub(mul(a[2],b[0]),mul(a[0],b[2])),sub(mul(a[0],b[1]),mul(a[1],b[0]))]
    def face(include):
        nonlocal state
        data,_=faceExpected(struct.pack('<I',include)+bytes(state)+struct.pack('<3f',*unit)+bytes(12));state=bytearray(data[:68]);return struct.unpack_from('<3f',data,68)
    normal=face(1)
    for iteration in range(100):
        length=f32(math.sqrt(add(add(mul(normal[0],normal[0]),mul(normal[1],normal[1])),mul(normal[2],normal[2]))));scale=f32(1/add(length,2**-23));normal=[mul(x,scale) for x in normal];opposite=[-x for x in normal];struct.pack_into('<3f',wire,108,*opposite);b=supportExpected(wire)
        if mode==0:wire[136:2186]=b[16:]
        featureB,*p=struct.unpack_from('<I3f',b);end=dot(ray[6:9],normal)>0;endpoint=ray[3:6] if end else ray[:3];feature=0;boxPoint=[0.,0.,0.]
        if not point:
            for axis in range(3):
                side=struct.unpack('<I',struct.pack('<f',normal[axis]))[0]>>31;feature|=side<<axis;boxPoint[axis]=axes[axis*2+side]
        incoming=[sub(add(boxPoint[j],endpoint[j]),p[j]) for j in range(3)];oldPoint=struct.unpack_from('<3f',state);projection=dot(incoming,normal);oldProjection=add(dot(oldPoint,normal),.00390625)
        if oldProjection>=projection:
            points=[struct.unpack_from('<3f',state,j*16) for j in range(3)];n=cross(offset(points[1],points[0]),offset(points[2],points[0]));denominator=dot(unit,n);value=f32(dot(points[0],n)/denominator) if denominator>0 else struct.unpack('<f',struct.pack('<I',0x6753c21c))[0];return result(value,iteration+1,'converged')
        struct.pack_into('<3fHHI',state,48,*incoming,(feature&0x7fff)|(0x8000 if end else 0),featureB&0xffff,4);normal=face(0)
    return result(0,100,'limit')
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior reference status');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        source=Path(e['source']);require(sha(source)==e['sha256'],'Original library changed');shutil.copyfile(source,libraries/source.name)
    executables=[]
    for name,machine in [('refinement_native_x86',0x14c),('refinement_capture_x86',0x14c),('refinement_owned_x64',0x8664),('refinement_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');data=p.read_bytes();pe=struct.unpack_from('<I',data,60)[0];require(struct.unpack_from('<H',data,pe+4)[0]==machine and b'asInvoker' in data,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    captured=[];observed=[];models=[];capturedModes=collections.Counter()
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior asset/query/trace changed');trace=case/'traces.bin';record=case/'captures.bin'
        p=subprocess.run([str(OUT/'refinement_capture_x86.exe'),str(libraries),m['phy'],str(queries),str(trace),str(record)],capture_output=True,text=True,timeout=45,cwd=case);(case/'capture.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Refinement capture failed '+str(case)+' '+p.stdout+p.stderr);require(trace.read_bytes()==old.read_bytes(),'Observation changed original trace');raw=record.read_bytes();at=0;calls=0
        while at<len(raw):
            require(at+4<=len(raw),'Missing query index');query=struct.unpack_from('<I',raw,at)[0];at+=4;require(query<m['queries'],'Captured query index');row,at=read_input(raw,at);answer,at=take(raw,at);captured.append(row);observed.append(answer);mode=struct.unpack_from('<I',row,164)[0];capturedModes[mode]+=1;calls+=1
        models.append(dict(model=m['model'],path=str(case),calls=calls,captures_sha256=sha(record),traces_sha256=sha(trace)));print('PASS original',index+1,'refinement='+str(calls),flush=True)
    require(captured,'No real refinement calls');rng=random.Random(0x28f40);rows=[]
    for mode,amount in [(2,4096),(1,128),(0,128)]:
        for j in range(amount):
            if mode==0:points,triangles=tetra()
            else:points=[[float(rng.randint(-4,4)) for _ in range(3)] for _ in range(8)];triangles=[]
            wire=encodeSupport(mode,points,(1,2,3),triangles=triangles,first=0,cache=([0]*1024+[65535]) if mode==0 else None)
            simplex=[[float(rng.randint(-8,8)) for _ in range(3)] for _ in range(4)]
            if j%17==0:simplex=[[float(k),0.,0.] for k in range(4)]
            ray=[f32(rng.randint(-4,4)/2) for _ in range(16)];point=j%2;box=[float(rng.randint(0,4)) if k%2==0 else float(rng.randint(-4,0)) for k in range(6)]
            rows.append(encode(simplex,ray,box,point,wire,rng))
    support=encodeSupport(2,[(0.,0.,0.)]*4,(1,2,3))
    for count in [0,1,2,3,0xffffffff]:rows.append(encode([(float('nan'),)*3]*4,[float('nan')]*16,[float('nan')]*6,1,support,rng,count))
    # A physically enclosing tetrahedron, signed-zero/zero segment variants.
    points=[(1.,1.,1.),(-1.,-1.,1.),(-1.,1.,-1.),(1.,-1.,-1.)]
    for point in [0,1]:
        for unit in [(0.,0.,0.),(-0.,0.,-0.),(1.,0.,0.),(0.,1.,0.),(0.,0.,1.)]:rows.append(encode(points,[0.]*9+list(unit)+[0.]*4,[1.,-1.]*3,point,encodeSupport(2,points,(1,2,3)),rng))
    controlled=len(rows);rows+=captured;inputs=run/'queries.bin';inputs.write_bytes(b''.join(rows));reference=[expected(row) for row in rows];(run/'expected.bin').write_bytes(b''.join(frame(x[0]) for x in reference));outputs=[];logs=[]
    for name in ['refinement_native_x86','refinement_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(inputs),str(dest)],capture_output=True,text=True,timeout=90);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Refinement kernel '+name+' '+logs[-1]);data=dest.read_bytes();at=0;parsed=[]
        while at<len(data):item,at=take(data,at);parsed.append(item)
        require(len(parsed)==len(rows),'Refinement output count');outputs.append(parsed)
    differences=[]
    for j,row in enumerate(rows):
        a,b=outputs[0][j],outputs[1][j];c=reference[j][0];actual=observed[j-controlled] if j>=controlled else a
        if not a==b==c==actual:
            differences.append(dict(case=j,captured=j>=controlled,input=row.hex(),native=a.hex(),owned=b.hex(),expected=c.hex(),observed=actual.hex()))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Refinement mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'refinement_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Refinement safety '+safety.stdout+safety.stderr)
    invalid=run/'invalid';invalid.mkdir();bad=[('empty',b'',True),('truncated_prefix',rows[0][:159],True),('truncated_support',rows[0][:-1],True)]
    for label,offset,bits in [('box_flag',156,2),('bad_mode',164,3),('nan_point',0,0x7fc00000),('nan_ray',68,0x7fc00000)]:
        row=bytearray(rows[0]);struct.pack_into('<I',row,offset,bits);bad.append((label,bytes(row),True))
    invalidChecks=0
    for label,data,both in bad:
        path=invalid/(label+'.bin');path.write_bytes(data)
        for name in ['refinement_owned_x64']+(['refinement_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(path),str(invalid/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Invalid refinement '+label+' '+name);invalidChecks+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_refinement',0x10028f40,0x100292a1),('complete_box_support',0x10027270,0x100272ce),('actual_refinement_call',0x10029e7e,0x10029e83),('complete_search_binding_constructor',0x100295f0,0x100296a6)])
    require(image.word(0x100ee018)==0x3b800000 and image.word(0x100c8090)==0x10027270,'Refinement convergence constant/box vtable')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    sources=['source_physics_refinement_candidate.h','physics_refinement_test_fixture.h','native_physics_refinement_fixture.h','probe_physics_refinement.cpp','probe_physics_refinement_capture.cpp','test_physics_refinement.cpp','build_physics_refinement.cmd','verify_physics_refinement.py','source_physics_refinement_face_candidate.h','source_physics_triangle_contact_time_candidate.h','source_physics_support_candidate.h','source_physics_line_simplex_candidate.h','physics_support_test_fixture.h','native_physics_support_fixture.h','native_vphysics_library_fixture.h','verify_physics_refinement_face.py','verify_physics_support.py','verify_ifm_draw_packet.py']
    result=dict(status='PASS',cases=len(rows),controlled_cases=controlled,captured_real_calls=len(captured),captured_support_modes=dict(capturedModes),differences=0,outcomes=dict(collections.Counter(x[2] for x in reference)),iteration_counts=dict(collections.Counter(x[1] for x in reference)),safety=safety.stdout+safety.stderr,invalid_fixture_checks=invalidChecks,executables=executables,logs=logs,models=models,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),query_sha256=sha(inputs),static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in sources},limits=['Complete28F40 and nested25460/25B10/A7F30/26D40/27270 execute intact in native direct and observed full trace calls; only CALL29E7E instrumented in loaded workspace copy','Owned complete refinement composes verified support, face selection, normalization, segment endpoint packing, box sign-bit support, convergence and final plane-time computation;72 result bytes plus2050 walk-cache bytes compared','Actual-call inputs include original prepared shape contexts and ray fields; full PHY-to-whole-search binding/cache lifetime and296B0/24ED0 caller integration remain unverified','Default floating controls/disjoint buffers/finite used data only; early return intentionally ignores opaque point/ray data','No editor rebuild/integration, original writes, renderer changes, UI/Blender launch or dependencies installed'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({key:result[key] for key in ['status','cases','controlled_cases','captured_real_calls','captured_support_modes','differences','outcomes','iteration_counts','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
