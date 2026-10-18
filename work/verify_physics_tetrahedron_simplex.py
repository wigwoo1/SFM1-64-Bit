"""Original tetrahedron-simplex calls from real physics traces vs original direct/x64/rules."""
import hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-tetrahedron-simplex-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(pass_,message):
    if not pass_:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def expected(wire,classified=False):
    from verify_physics_triangle_simplex import expected as triangle
    state=bytearray(wire[:68]);p=struct.unpack_from('<3f',wire,68)
    mul=lambda x,y:f32(x*y)
    sub=lambda x,y:f32(x-y)
    def cross(u,v):return [sub(mul(u[1],v[2]),mul(u[2],v[1])),sub(mul(u[2],v[0]),mul(u[0],v[2])),sub(mul(u[0],v[1]),mul(u[1],v[0]))]
    dot=lambda v:f32(f32(mul(p[1],v[1])+mul(p[0],v[0]))+mul(p[2],v[2]))
    a,b,c=([sub(x,y) for x,y in zip(struct.unpack_from('<3f',wire,at),p)] for at in [0,16,32])
    ab=cross(a,b);bc=cross(b,c);ca=cross(c,a);abB=cross(ab,b);abA=cross(a,ab);bcC=cross(bc,c);bcB=cross(b,bc);caA=cross(ca,a);caC=cross(c,ca)
    mask=(1 if dot(ab)<0 else 0)|(2 if dot(bc)<0 else 0)|(4 if dot(ca)<0 else 0)
    label=None;complete=0
    if mask==0:
        label='enclosing';state[48:64]=wire[68:84];struct.pack_into('<I',state,64,4);result=bytes(state)+wire[84:96]+struct.pack('<I',1)
        return (result,(mask,label)) if classified else result
    if mask in [1,2,4]:
        if mask==2:state[:16]=wire[32:48]
        if mask==4:state[16:32]=wire[32:48]
        result,branch=triangle(bytes(state)+wire[68:],True)
        return (result,(mask,'triangle_'+branch)) if classified else result
    edge=None;face=None
    if mask==3:
        if dot(abB)<0:
            if dot(bcB)<0:edge=1 if dot(b)<0 else None
            elif dot(bcC)<0:edge=2
            else:face=1
        elif dot(abA)<0:edge=0 if dot(a)<0 else None
        else:face=0
    elif mask==5:
        if dot(caA)<0:
            if dot(abA)<0:edge=0 if dot(a)<0 else None
            elif dot(abB)<0:edge=1
            else:face=0
        elif dot(caC)<0:edge=2 if dot(c)<0 else None
        else:face=2
    elif mask==6:
        if dot(bcC)<0:
            if dot(caC)<0:edge=2 if dot(c)<0 else None
            elif dot(caA)<0:edge=0
            else:face=2
        elif dot(bcB)<0:edge=1 if dot(b)<0 else None
        else:face=1
    else:
        edge=next((j for j,v in enumerate([a,b,c]) if dot(v)<0),None)
    if edge is not None:
        label='edge_'+str(edge);d=[a,b,c][edge];struct.pack_into('<I',state,64,2)
        if edge==1:state[:16]=wire[68:84]
        else:
            if edge==2:state[:16]=wire[32:48]
            state[16:32]=wire[68:84]
        t0=sub(mul(p[2],d[1]),mul(p[1],d[2]));t1=sub(mul(p[0],d[2]),mul(p[2],d[0]));t2=sub(mul(p[1],d[0]),mul(p[0],d[1]))
        direction=[sub(mul(t1,d[2]),mul(t2,d[1])),sub(mul(t2,d[0]),mul(t0,d[2])),sub(mul(t0,d[1]),mul(t1,d[0]))]
    elif face is not None:
        label='face_'+str(face);struct.pack_into('<I',state,64,3);at=[32,0,16][face];state[at:at+16]=wire[68:84];direction=[-v for v in [ab,bc,ca][face]]
    else:
        label='point';struct.pack_into('<I',state,64,1);state[:16]=wire[68:84];direction=p
    result=bytes(state)+struct.pack('<3fI',*direction,complete)
    return (result,(mask,label)) if classified else result

def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior original trace receipt');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        source=Path(e['source']);require(sha(source)==e['sha256'],'Original library changed');shutil.copyfile(source,libraries/source.name)
    executables=[]
    for name,machine in [('simplex_native_x86',0x14c),('simplex_capture_x86',0x14c),('simplex_owned_x64',0x8664),('simplex_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(b[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Executable architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    captured=[];captureOutputs=[];modelEvidence=[]
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior PHY/query/trace provenance');trace=case/'traces.bin';record=case/'captures.bin';p=subprocess.run([str(OUT/'simplex_capture_x86.exe'),str(libraries),m['phy'],str(queries),str(trace),str(record)],capture_output=True,text=True,timeout=30,cwd=case);(case/'capture.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Capture failed '+str(case)+' '+p.stdout+p.stderr);require(trace.read_bytes()==old.read_bytes(),'Observers changed full original trace output');raw=record.read_bytes();require(len(raw)%184==0,'Capture record extent')
        for i in range(len(raw)//184):
            at=i*184;q=struct.unpack_from('<I',raw,at)[0];require(q<m['queries'],'Captured query index');captured.append(raw[at+4:at+100]);captureOutputs.append(raw[at+100:at+184])
        modelEvidence.append(dict(model=m['model'],phy=m['phy'],phy_sha256=m['phy_sha256'],queries=m['queries'],tetrahedron_calls=len(raw)//184,captures_sha256=sha(record),traces_sha256=sha(trace),path=str(case)));print('PASS observed',index+1,m['model'],'calls='+str(len(raw)//184),flush=True)
    require(len(captured)>0,'No real simplex calls observed');rows=[];rng=random.Random(0x27c00)
    def add(a,b,c,new,features=None):
        wire=bytearray(struct.pack('<24I',*[rng.getrandbits(32) for _ in range(24)]));struct.pack_into('<3f',wire,0,*a);struct.pack_into('<3f',wire,16,*b);struct.pack_into('<3f',wire,32,*c);struct.pack_into('<3f',wire,68,*new)
        if features is not None:
            for at,pair in [(12,features),(28,features[::-1]),(44,features),(80,features[::-1])]:struct.pack_into('<2H',wire,at,*pair)
        rows.append(bytes(wire))
    directed=[((1,0,0),(0,1,0),(0,0,1),(-1,-1,-1)),((2,0,0),(3,0,0),(4,0,0),(1,0,0)),((0,0,0),(0,0,0),(0,0,0),(0,0,0)),((-0.,0,-0.),(0,-0.,0),(-0.,-0.,0),(0.,0.,-0.)),((1,2,3),(1,2,3),(1,2,3),(1,2,3))]
    for a,b,c,new in directed:
        for fa in [0,1,0x7fff,0x8000,0xffff]:
            for fb in [0,1,0x7fff,0x8000,0xffff]:add(a,b,c,new,(fa,fb))
    for i in range(32768):
        scale=rng.choice([1e-6,.01,1,128,8192]);points=[[f32(rng.uniform(-1,1)*scale) for _ in range(3)] for _ in range(4)];add(*points)
    controlled=len(rows);rows+=captured;query=run/'queries.bin';query.write_bytes(b''.join(rows));reference=b''.join(expected(row) for row in rows);(run/'independent_expected.bin').write_bytes(reference);outputs=[];logs=[]
    for name in ['simplex_native_x86','simplex_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(query),str(dest)],capture_output=True,text=True,timeout=30);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Direct simplex failed '+name+' '+logs[-1]);data=dest.read_bytes();require(len(data)==len(rows)*84,'Simplex result extent');outputs.append(data)
    differences=[]
    for i,row in enumerate(rows):
        a,b,c=(x[i*84:(i+1)*84] for x in [*outputs,reference]);observed=captureOutputs[i-controlled] if i>=controlled else a
        if not a==b==c==observed:
            differences.append(dict(case=i,captured=i>=controlled,input=row.hex(),native=a.hex(),owned=b.hex(),expected=c.hex(),observed=observed.hex()))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Tetrahedron simplex mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'simplex_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Safety '+safety.stdout+safety.stderr)
    bad=[]
    for label,at,bits in [('nan_old',0,0x7fc00000),('inf_old1',16,0x7f800000),('nan_old2',32,0x7fc00000),('inf_new',68,0x7f800000)]:
        b=bytearray(rows[0]);struct.pack_into('<I',b,at,bits);bad.append((label,bytes(b),True))
    bad.append(('truncated',rows[0][:-1],True));b=bytearray(rows[0]);struct.pack_into('<3f',b,0,1e20,0,0);struct.pack_into('<3f',b,68,-1e20,0,0);bad.append(('projection_overflow',bytes(b),False));badDir=run/'invalid';badDir.mkdir();invalid=0
    for label,b,both in bad:
        q=badDir/(label+'.bin');q.write_bytes(b)
        for name in ['simplex_owned_x64']+(['simplex_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(q),str(badDir/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Malformed fixture did not reject normally');invalid+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_tetrahedron_simplex',0x10027c00,0x10028f18),('complete_nested_triangle_simplex',0x10027710,0x10027bf1),('complete_feature_vertex_copy',0x10024e70,0x10024ec5)]+[(f'original_tetrahedron_call_{site:x}',site,site+5) for site in [0x1002993c,0x10029a72]])
    require(image.word(0x100c52d0)==0x80000000,'Original normal sign flip mask');require([image.word(0x10028f18+j*4) for j in range(8)]==[0x1002808b,0x100280a6,0x100280bb,0x100280f6,0x100280d8,0x1002861a,0x10028a01,0x10028d71],'Original tetrahedron eight-way switch table')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy library changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Post-comparison original asset changed')
    from collections import Counter
    classified=[expected(row,True)[1] for row in rows];masks=Counter(mask for mask,label in classified);require(set(masks)==set(range(8)),'Missing tetrahedron mask coverage');coverage={str(mask):count for mask,count in sorted(masks.items())};outcomes=dict(Counter(label for mask,label in classified));require(all(any(label=='edge_'+str(j) for mask,label in classified) for j in range(3)),'Missing direct edge coverage');require(all(any(label=='face_'+str(j) for mask,label in classified) for j in range(3)),'Missing direct face coverage')
    receipt=dict(status='PASS',cases=len(rows),controlled_cases=controlled,captured_real_calls=len(captured),bytes_per_result=84,differences=0,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,coverage=coverage,outcomes=outcomes,safety=safety.stdout+safety.stderr,invalid_fixture_checks=invalid,executables=executables,logs=logs,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),models=modelEvidence,query_sha256=sha(query),static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in ['source_physics_tetrahedron_simplex_candidate.h','source_physics_triangle_simplex_candidate.h','source_physics_line_simplex_candidate.h','verify_physics_triangle_simplex.py','physics_line_simplex_test_fixture.h','probe_physics_tetrahedron_simplex.cpp','probe_physics_tetrahedron_simplex_capture.cpp','test_physics_tetrahedron_simplex.cpp','build_physics_tetrahedron_simplex.cmd','verify_physics_tetrahedron_simplex.py','native_vphysics_library_fixture.h']},limits=['Complete original27C00 plus intact27710/24E70 execute; owned implements full finite-result tetrahedron reduction and delegates its three single-face cases to previously verified triangle reducer','Both real GJK tetrahedron call sites observed through bridges in loaded workspace DLL copies; full TraceBox answers unchanged for7381queries/61shapes','Original enclosing case returns true/count4, copies incoming toslot3 and preserves old direction even if nonfinite; degenerate geometry follows original branch order, not geometric reinterpretation','Captured unused bytes remain opaque; arbitrary input count, vertex3 and old direction are not valid geometric inputs','No owned support mapping, complete GJK/compact-tree tracing/hit-normal/startsolid or live/editor integration yet','Default floating-point controls tested; no alias/liveABI claim; no original writes/UI/editorbuild/dependencyinstalls/idlehelpers'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','cases','controlled_cases','captured_real_calls','differences','coverage','outcomes','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
