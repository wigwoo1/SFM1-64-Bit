"""Original line-simplex calls from real physics traces vs original direct/x64/rules."""
import hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-line-simplex-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(pass_,message):
    if not pass_:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def expected(wire):
    state=bytearray(wire[:68]);old=struct.unpack_from('<3f',wire);p=struct.unpack_from('<3f',wire,68);d=[f32(a-b) for a,b in zip(old,p)]
    mul=lambda a,b:f32(a*b)
    sub=lambda a,b:f32(a-b)
    dot=f32(f32(mul(p[1],d[1])+mul(p[0],d[0]))+mul(p[2],d[2]))
    if dot<0:
        state[16:32]=wire[68:84];struct.pack_into('<I',state,64,2);a=sub(mul(p[2],d[1]),mul(p[1],d[2]));b=sub(mul(p[1],d[0]),mul(p[0],d[1]));c=sub(mul(p[0],d[2]),mul(p[2],d[0]));direction=[sub(mul(d[2],c),mul(b,d[1])),sub(mul(d[0],b),mul(a,d[2])),sub(mul(a,d[1]),mul(d[0],c))]
    else:state[:16]=wire[68:84];struct.pack_into('<I',state,64,1);direction=p
    return bytes(state)+struct.pack('<3fI',*direction,0)
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
        modelEvidence.append(dict(model=m['model'],phy=m['phy'],phy_sha256=m['phy_sha256'],queries=m['queries'],line_calls=len(raw)//184,captures_sha256=sha(record),traces_sha256=sha(trace),path=str(case)));print('PASS observed',index+1,m['model'],'calls='+str(len(raw)//184),flush=True)
    require(len(captured)>0,'No real simplex calls observed');rows=[];rng=random.Random(0x27580)
    def add(old,new,features=None):
        wire=bytearray(struct.pack('<24I',*[rng.getrandbits(32) for _ in range(24)]));struct.pack_into('<3f',wire,0,*old);struct.pack_into('<3f',wire,68,*new)
        if features is not None:struct.pack_into('<2H',wire,80,*features)
        rows.append(bytes(wire))
    for old,new in [((2,0,0),(1,0,0)),((1,0,0),(-1,1,0)),((1,0,0),(-1,0,0)),((1,2,3),(1,2,3)),((0,0,0),(0,0,0)),((-0.,0,-0.),(0,-0.,0)),((1,0,0),(0,1,0))]:
        for a in [0,1,0x7fff,0x8000,0xffff]:
            for b in [0,1,0x7fff,0x8000,0xffff]:add(old,new,(a,b))
    for i in range(16384):
        scale=rng.choice([1e-6,.01,1,128,8192]);old=[f32(rng.uniform(-1,1)*scale) for _ in range(3)];new=[f32(rng.uniform(-1,1)*scale) for _ in range(3)];add(old,new)
    controlled=len(rows);rows+=captured;query=run/'queries.bin';query.write_bytes(b''.join(rows));reference=b''.join(expected(row) for row in rows);(run/'independent_expected.bin').write_bytes(reference);outputs=[];logs=[]
    for name in ['simplex_native_x86','simplex_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(query),str(dest)],capture_output=True,text=True,timeout=30);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Direct simplex failed '+name+' '+logs[-1]);data=dest.read_bytes();require(len(data)==len(rows)*84,'Simplex result extent');outputs.append(data)
    differences=[]
    for i,row in enumerate(rows):
        a,b,c=(x[i*84:(i+1)*84] for x in [*outputs,reference]);observed=captureOutputs[i-controlled] if i>=controlled else a
        if not a==b==c==observed:
            differences.append(dict(case=i,captured=i>=controlled,input=row.hex(),native=a.hex(),owned=b.hex(),expected=c.hex(),observed=observed.hex()))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Line simplex mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'simplex_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Safety '+safety.stdout+safety.stderr)
    bad=[]
    for label,at,bits in [('nan_old',0,0x7fc00000),('inf_new',68,0x7f800000)]:
        b=bytearray(rows[0]);struct.pack_into('<I',b,at,bits);bad.append((label,bytes(b),True))
    bad.append(('truncated',rows[0][:-1],True));b=bytearray(rows[0]);struct.pack_into('<3f',b,0,1e20,0,0);struct.pack_into('<3f',b,68,-1e20,0,0);bad.append(('projection_overflow',bytes(b),False));badDir=run/'invalid';badDir.mkdir();invalid=0
    for label,b,both in bad:
        q=badDir/(label+'.bin');q.write_bytes(b)
        for name in ['simplex_owned_x64']+(['simplex_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(q),str(badDir/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Malformed fixture did not reject normally');invalid+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_two_point_simplex',0x10027580,0x10027704),('first_real_GJK_call',0x10029916,0x10029923),('second_real_GJK_call',0x10029a4c,0x10029a59)])
    require(image.word(0x100c8084)==0x10026d40,'Original motion support vtable (next unresolved stage)')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy library changed')
    coverage=dict(point=sum(struct.unpack_from('<I',reference,i*84+64)[0]==1 for i in range(len(rows))),edge=sum(struct.unpack_from('<I',reference,i*84+64)[0]==2 for i in range(len(rows))),zero_direction=sum(all(v==0 for v in struct.unpack_from('<3f',reference,i*84+68)) for i in range(len(rows))))
    receipt=dict(status='PASS',cases=len(rows),controlled_cases=controlled,captured_real_calls=len(captured),bytes_per_result=84,differences=0,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,coverage=coverage,safety=safety.stdout+safety.stderr,invalid_fixture_checks=invalid,executables=executables,logs=logs,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),models=modelEvidence,query_sha256=sha(query),static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in ['source_physics_line_simplex_candidate.h','physics_line_simplex_test_fixture.h','probe_physics_line_simplex.cpp','probe_physics_line_simplex_capture.cpp','test_physics_line_simplex.cpp','build_physics_line_simplex.cmd','verify_physics_line_simplex.py','native_vphysics_library_fixture.h']},limits=['Complete original27580 executes without math callbacks; owned implements same finite-result scalar routine, not full GJK/physics tracing','Both real GJK calls observed in loaded workspace DLL by bridges at2991E/29A54 calling intact original; original full TraceBox answers unchanged on7381queries/61shapes','Captured unused simplex bytes may be uninitialized native stack data; treated as opaque retained bytes, never meaningful coordinates/identities','Native captures/direct original calls/owned AMD64/independent float-step rules all agree for recorded calls','Only vertex0/incoming point used; input count/unused vertices/input direction intentionally unchecked and retained/overwritten as original','No owned support mapping, triangle/tetrahedron reduction, full hit selection, inside-solid or editor integration yet','No original writes, editor changes/build, app UI launches, installs or idle helper processes'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','cases','controlled_cases','captured_real_calls','differences','coverage','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
