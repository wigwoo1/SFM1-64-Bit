"""Complete original24ED0 contact-plane policy and vertex getters vs owned x64."""
import collections,hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_physics_refinement import encode as encodeRefinement
from verify_physics_support import encode as encodeSupport,tetra
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-contact-plane-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def take(data,at):
    require(at+4<=len(data),'Missing support frame');n=struct.unpack_from('<I',data,at)[0];require(at+4+n<=len(data),'Truncated support frame');return data[at+4:at+4+n],at+4+n
def read_input(data,at):
    start=at;require(at+160<=len(data),'Truncated contact prefix');_,at=take(data,at+160);require(at+84<=len(data),'Truncated initial trace');at+=84;return data[start:at],at
def expected(row):
    wire,at=take(row,160);trace=bytearray(row[at:at+84]);count=struct.unpack_from('<i',row,64)[0];a=[];b=[]
    for j in range(max(1,count)):
        fa,fb=struct.unpack_from('<HH',row,j*16+12);fa&=0x7fff
        if fa not in a:a.append(fa)
        if fb not in b:b.append(fb)
    ray=struct.unpack_from('<16f',row,68);axes=struct.unpack_from('<6f',row,132);traceEnd=struct.unpack_from('<3f',trace,12);mode,n,t,first,_,_=struct.unpack_from('<6I',wire);transform=struct.unpack_from('<12f',wire,60);pointAt=136+(2050 if mode==0 else 0)
    mul=lambda a,b:f32(a*b);sub=lambda a,b:f32(a-b);add=lambda a,b:f32(a+b)
    def difference(a,b):return [sub(a[j],b[j]) for j in range(3)]
    def cross(a,b):return [sub(mul(a[1],b[2]),mul(a[2],b[1])),sub(mul(a[2],b[0]),mul(a[0],b[2])),sub(mul(a[0],b[1]),mul(a[1],b[0]))]
    def dot(a,b):return add(add(mul(a[1],b[1]),mul(a[0],b[0])),mul(a[2],b[2]))
    def normalize(v):
        length=f32(math.sqrt(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]))));scale=f32(1/add(length,2**-23));return [mul(x,scale) for x in v]
    def vertexA(feature):return [axes[axis*2+((feature>>axis)&1)] for axis in range(3)]
    def vertexB(feature):
        index=feature-first if mode==1 else feature;require(0<=index<n,'Reference feature range');p=struct.unpack_from('<3f',wire,pointAt+index*12)
        return p if mode==2 else [add(add(add(mul(transform[j*4+1],p[1]),mul(transform[j*4],p[0])),mul(transform[j*4+2],p[2])),transform[j*4+3]) for j in range(3)]
    def orient(v):return [-x for x in v] if dot(ray[6:9],v)>0 else v
    def aDistance(p,normal):return dot([add(traceEnd[j],p[j]) for j in range(3)],normal)
    if count==2 and len(a)==2:
        p,q=[vertexA(x) for x in a];edge=difference(q,p);normal=normalize(cross(edge,cross(edge,ray[6:9])));distance=aDistance(p,normal);branch='A_edge'
    elif len(a)==3:
        p,q,r=[vertexA(x) for x in a];normal=orient(normalize(cross(difference(q,p),difference(r,p))));distance=aDistance(p,normal);branch='A_face'
    elif len(a)==2 and len(b)==2:
        p,q=[vertexA(x) for x in a];r,s=[vertexB(x) for x in b];normal=orient(normalize(cross(difference(q,p),difference(s,r))));distance=aDistance(p,normal);branch='edge_edge'
    elif len(b)==3:
        p,q,r=[vertexB(x) for x in b];normal=orient(normalize(cross(difference(q,p),difference(r,p))));distance=dot(p,normal);branch='B_face'
    else:
        normal=[-x for x in ray[9:12]];distance=dot(vertexB(b[0]),normal) if count else 0.;branch='fallback_nonzero' if count else 'fallback_zero'
    struct.pack_into('<4f',trace,24,*normal,distance);return bytes(trace),branch
def make_row(mode,count,fa,fb,rng):
    if mode==0:points,triangles=tetra()
    else:points=[[f32(rng.randint(-8,8)*rng.choice([2**-10,1,32])) for _ in range(3)] for _ in range(8)];triangles=[]
    first=rng.randint(0,32) if mode==1 else 0;matrix=[f32(rng.randint(-4,4)/2) for _ in range(12)];wire=encodeSupport(mode,points,(1,2,3),transform=matrix,first=first,triangles=triangles,cache=[0xa7]*1024+[65535] if mode==0 else None)
    ray=[f32(rng.randint(-8,8)/2) for _ in range(16)];axes=[float(rng.randint(0,8)) if k%2==0 else float(rng.randint(-8,0)) for k in range(6)];row=bytearray(encodeRefinement([(float('nan'),)*3]*4,ray,axes,rng.randrange(2),wire,rng,count));stateCount=max(1,count if count<0x80000000 else 0)
    for j in range(stateCount):struct.pack_into('<HH',row,j*16+12,fa[j]|(rng.randrange(2)<<15),first+fb[j])
    trace=bytearray(rng.getrandbits(8) for _ in range(84));struct.pack_into('<3f',trace,12,*[float(rng.randint(-8,8)) for _ in range(3)]);return bytes(row)+trace
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior trace status');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        source=Path(e['source']);require(sha(source)==e['sha256'],'Original library changed');shutil.copyfile(source,libraries/source.name)
    executables=[]
    for name,machine in [('plane_native_x86',0x14c),('plane_capture_x86',0x14c),('plane_owned_x64',0x8664),('plane_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');data=p.read_bytes();pe=struct.unpack_from('<I',data,60)[0];require(struct.unpack_from('<H',data,pe+4)[0]==machine and b'asInvoker' in data,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    captured=[];observed=[];models=[]
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior asset/query/trace changed');trace=case/'traces.bin';record=case/'captures.bin'
        p=subprocess.run([str(OUT/'plane_capture_x86.exe'),str(libraries),m['phy'],str(queries),str(trace),str(record)],capture_output=True,text=True,timeout=45,cwd=case);(case/'capture.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Plane capture failed '+str(case)+' '+p.stdout+p.stderr);require(trace.read_bytes()==old.read_bytes(),'Contact observation changed original trace');raw=record.read_bytes();at=0;calls=0
        while at<len(raw):
            require(at+4<=len(raw),'Missing capture query');query=struct.unpack_from('<I',raw,at)[0];at+=4;require(query<m['queries'],'Capture query index');row,at=read_input(raw,at);require(at+84<=len(raw),'Missing captured output trace');captured.append(row);observed.append(raw[at:at+84]);at+=84;calls+=1
        models.append(dict(model=m['model'],path=str(case),calls=calls,captures_sha256=sha(record),traces_sha256=sha(trace)));print('PASS original',index+1,'contact-plane='+str(calls),flush=True)
    require(captured,'No real contact-plane calls');rng=random.Random(0x24ed0);rows=[]
    for mode in [0,1,2]:
        for j in range(4096):
            count=rng.choice([1,2,3,4]);fa=[rng.randrange(8) for _ in range(count)];fb=[rng.randrange(4) for _ in range(count)];rows.append(make_row(mode,count,fa,fb,rng))
        for count,fa,fb in [(0,[0],[0]),(0xffffffff,[0],[0]),(2,[0,1],[0,0]),(3,[0,1,2],[0,0,0]),(3,[0,1,1],[0,1,1]),(3,[0,0,0],[0,1,2]),(4,[0,1,2,3],[0,1,2,3])]:rows.append(make_row(mode,count,fa,fb,rng))
    # Endpoint flag alone must not turn one A vertex into a distinct feature.
    row=bytearray(make_row(2,2,[0,0],[0,1],rng));struct.pack_into('<H',row,12,0);struct.pack_into('<H',row,28,0x8000);rows.append(bytes(row))
    # Opaque ray start/end and unused simplex point bytes must not be read.
    row=bytearray(make_row(2,1,[0],[1],rng))
    for at in range(68,92,4):struct.pack_into('<I',row,at,0x7fc00000)
    rows.append(bytes(row));controlled=len(rows);rows+=captured;inputs=run/'queries.bin';inputs.write_bytes(b''.join(rows));reference=[expected(row) for row in rows];(run/'expected.bin').write_bytes(b''.join(x[0] for x in reference));outputs=[];logs=[]
    for name in ['plane_native_x86','plane_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(inputs),str(dest)],capture_output=True,text=True,timeout=60);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Plane direct '+name+' '+logs[-1]);data=dest.read_bytes();require(len(data)==len(rows)*84,'Plane result extent');outputs.append(data)
    differences=[]
    for j,row in enumerate(rows):
        a,b=outputs[0][j*84:(j+1)*84],outputs[1][j*84:(j+1)*84];c=reference[j][0];actual=observed[j-controlled] if j>=controlled else a
        if not a==b==c==actual:
            differences.append(dict(case=j,captured=j>=controlled,input=row.hex(),native=a.hex(),owned=b.hex(),expected=c.hex(),observed=actual.hex()))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Plane mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'plane_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Contact plane safety '+safety.stdout+safety.stderr)
    invalid=run/'invalid';invalid.mkdir();bad=[('empty',b'',True),('truncated_prefix',rows[0][:159],True),('truncated_trace',rows[0][:-1],True)]
    for label,at,bits in [('bad_count',64,5),('bad_box_flag',156,2),('bad_mode',164,3)]:
        row=bytearray(rows[0]);struct.pack_into('<I',row,at,bits);bad.append((label,bytes(row),True))
    row=bytearray(make_row(2,1,[0],[0],rng));struct.pack_into('<I',row,104,0x7fc00000);bad.append(('nan_used_unit',bytes(row),False));row=bytearray(make_row(2,1,[0],[0],rng));struct.pack_into('<H',row,14,65535);bad.append(('bad_B_feature',bytes(row),False));invalidChecks=0
    for label,data,both in bad:
        path=invalid/(label+'.bin');path.write_bytes(data)
        for name in ['plane_owned_x64']+(['plane_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(path),str(invalid/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Invalid plane fixture '+label+' '+name);invalidChecks+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_contact_plane',0x10024ed0,0x1002545b),('complete_geometry_vertex_getter',0x100271e0,0x1002725a),('complete_box_vertex_getter',0x100272d0,0x10027305),('actual_plane_call',0x10029ebd,0x10029ec2)])
    require(image.word(0x100c8088)==0x100271e0 and image.word(0x100c8094)==0x100272d0,'Original vertex getter slots')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    sources=['source_physics_contact_plane_candidate.h','physics_contact_plane_test_fixture.h','native_physics_contact_plane_fixture.h','probe_physics_contact_plane.cpp','probe_physics_contact_plane_capture.cpp','test_physics_contact_plane.cpp','build_physics_contact_plane.cmd','verify_physics_contact_plane.py','source_physics_refinement_candidate.h','physics_refinement_test_fixture.h','native_physics_refinement_fixture.h','source_physics_support_candidate.h','physics_support_test_fixture.h','native_physics_support_fixture.h','native_vphysics_library_fixture.h','source_physics_triangle_contact_time_candidate.h','verify_physics_refinement.py','verify_physics_refinement_face.py','verify_physics_support.py','verify_ifm_draw_packet.py']
    result=dict(status='PASS',cases=len(rows),controlled_cases=controlled,captured_real_calls=len(captured),differences=0,branches=dict(collections.Counter(x[1] for x in reference)),captured_branches=dict(collections.Counter(x[1] for x in reference[controlled:])),safety=safety.stdout+safety.stderr,invalid_fixture_checks=invalidChecks,executables=executables,logs=logs,models=models,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),query_sha256=sha(inputs),static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in sources},limits=['Complete24ED0/271E0/272D0 execute intact in original DLL; custom usercall oracle preserves ECX/EDX/EDI and caller-owned stack; only CALL29EBD instrumented in memory','All84 trace bytes compared,16 plane bytes changed and68 other bytes retained; input simplex/ray/box/geometry/cache unchanged','A feature dedup masks endpoint bit; B feature words unmasked; all original branch priorities and epsilon normalization retained','Prepared original contexts and ray fields are test inputs; full296B0 search/file binding/caller hit conversion/filter/runtime/editor integration remain incomplete','Only finite used data/disjoint buffers/default floating controls verified; ignored input bytes remain opaque; no original nonfinite-behavior claim','No original writes, editor build/integration, rendering/animation/serialization changes, UI/Blender launch or dependencies installed'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','controlled_cases','captured_real_calls','differences','branches','captured_branches','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
