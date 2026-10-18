"""Complete original25F50 vs owned AMD64 with real trace-call observations."""
import collections,hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-triangle-contact-time-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def encode(points,direction,tolerance,rng):
    # Unused vertex3/count/feature words deliberately arbitrary opaque bytes.
    row=bytearray(rng.getrandbits(8) for _ in range(84))
    for j,p in enumerate(points):struct.pack_into('<3f',row,j*16,*p)
    struct.pack_into('<4f',row,68,*direction,tolerance);return bytes(row)
def expected(row):
    p=[struct.unpack_from('<3f',row,j*16) for j in range(3)];d=struct.unpack_from('<3f',row,68);t=struct.unpack_from('<f',row,80)[0]
    mul=lambda a,b:f32(a*b);add=lambda a,b:f32(a+b);sub=lambda a,b:f32(a-b)
    def subtract(a,b):return [sub(a[j],b[j]) for j in range(3)]
    def cross(a,b):return [sub(mul(a[1],b[2]),mul(a[2],b[1])),sub(mul(a[2],b[0]),mul(a[0],b[2])),sub(mul(a[0],b[1]),mul(a[1],b[0]))]
    def normalize(v):
        length=f32(math.sqrt(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]))));scale=f32(1/add(length,2**-23));return [mul(x,scale) for x in v]
    def dot(a,b):return add(add(mul(a[1],b[1]),mul(a[0],b[0])),mul(a[2],b[2]))
    a,b,c=p;u=subtract(a,c);v=subtract(b,c);w=subtract(b,a);normal=normalize(cross(u,v));sides=[normalize(cross(u,normal)),normalize(cross(normal,v)),normalize(cross(w,normal))];denominators=[dot(d,s) for s in sides];sentinel=struct.unpack('<f',struct.pack('<I',0x6753c21c))[0];times=[f32(dot(a if j==0 else b,sides[j])/denominators[j]) if denominators[j]>0 else sentinel for j in range(3)];chosen=0
    if times[0]>times[1]:chosen=1
    if times[chosen]>times[2]:chosen=2
    value=t/denominators[chosen]+times[chosen] if denominators[chosen]>0 else sentinel
    roundedFirst=f32(f32(t/denominators[chosen])+times[chosen]) if denominators[chosen]>0 else sentinel
    return struct.pack('<f',value),chosen,denominators[chosen]>0,struct.pack('<f',roundedFirst)!=struct.pack('<f',value)
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior trace status');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        source=Path(e['source']);require(sha(source)==e['sha256'],'Original library changed');shutil.copyfile(source,libraries/source.name)
    executables=[]
    for name,machine in [('contact_native_x86',0x14c),('contact_capture_x86',0x14c),('contact_owned_x64',0x8664),('contact_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');data=p.read_bytes();pe=struct.unpack_from('<I',data,60)[0];require(struct.unpack_from('<H',data,pe+4)[0]==machine and b'asInvoker' in data,'Architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    captured=[];observed=[];models=[]
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior model/query/trace changed');trace=case/'traces.bin';record=case/'captures.bin'
        p=subprocess.run([str(OUT/'contact_capture_x86.exe'),str(libraries),m['phy'],str(queries),str(trace),str(record)],capture_output=True,text=True,timeout=45,cwd=case);(case/'capture.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Contact capture failed '+str(case)+' '+p.stdout+p.stderr);require(trace.read_bytes()==old.read_bytes(),'Contact observation changed original trace');raw=record.read_bytes();require(len(raw)%92==0,'Contact capture extent')
        for at in range(0,len(raw),92):
            require(struct.unpack_from('<I',raw,at)[0]<m['queries'],'Contact query index');captured.append(raw[at+4:at+88]);observed.append(raw[at+88:at+92])
        models.append(dict(model=m['model'],path=str(case),calls=len(raw)//92,captures_sha256=sha(record),traces_sha256=sha(trace)));print('PASS original',index+1,'calls='+str(len(raw)//92),flush=True)
    rng=random.Random(0x25f50);rows=[]
    for j in range(16384):
        scale=rng.choice([2**-12,1,128]);points=[[f32(rng.randint(-8,8)*scale) for _ in range(3)] for _ in range(3)]
        if j%11==0:points[2]=points[0][:]
        if j%17==0:points=[[x,0.,0.] for x in [-scale,0.,scale]]
        direction=[f32(rng.randint(-8,8)*rng.choice([2**-20,1,128])) for _ in range(3)];tolerance=f32(rng.choice([0.,-.03125,.03125,1e-8,2.]))
        rows.append(encode(points,direction,tolerance,rng))
    triangle=[(0.,0.,0.),(1.,0.,0.),(0.,1.,0.)]
    for d in [(0.,0.,0.),(-0.,0.,-0.),(0.,0.,1.),(1.,0.,0.),(0.,1.,0.),(-1.,-1.,0.)]:
        for t in [-1.,-0.,0.,2**-23,.03125,1.]:rows.append(encode(triangle,d,t,rng))
    controlled=len(rows);rows+=captured;inputs=run/'queries.bin';inputs.write_bytes(b''.join(rows));reference=[expected(row) for row in rows];(run/'expected.bin').write_bytes(b''.join(x[0] for x in reference));outputs=[];logs=[]
    for name in ['contact_native_x86','contact_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(inputs),str(dest)],capture_output=True,text=True,timeout=60);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Contact kernel '+name+' '+logs[-1]);data=dest.read_bytes();require(len(data)==len(rows)*4,'Contact output extent');outputs.append(data)
    require('x87 precision=65536' in logs[0],'Original runtime is not default53bit x87 precision')
    differences=[]
    for j,row in enumerate(rows):
        a,b=outputs[0][j*4:j*4+4],outputs[1][j*4:j*4+4];c=reference[j][0];actual=observed[j-controlled] if j>=controlled else a
        if not a==b==c==actual:
            differences.append(dict(case=j,captured=j>=controlled,input=row.hex(),native=a.hex(),owned=b.hex(),expected=c.hex(),observed=actual.hex()))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Contact mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'contact_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Safety '+safety.stdout+safety.stderr)
    invalid=run/'invalid';invalid.mkdir();bad=[('empty',b'',True),('truncated',rows[0][:-1],True)]
    for label,offset,value in [('nan_point',0,0x7fc00000),('inf_direction',68,0x7f800000),('nan_tolerance',80,0x7fc00000)]:
        row=bytearray(rows[0]);struct.pack_into('<I',row,offset,value);bad.append((label,bytes(row),True))
    row=bytearray(rows[0]);struct.pack_into('<3f',row,0,3e38,0,0);struct.pack_into('<3f',row,16,0,3e38,0);struct.pack_into('<3f',row,32,0,0,3e38);bad.append(('overflow',bytes(row),False));invalidChecks=0
    for label,data,both in bad:
        path=invalid/(label+'.bin');path.write_bytes(data)
        for name in ['contact_owned_x64']+(['contact_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(path),str(invalid/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Malformed fixture failure '+label+' '+name);invalidChecks+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_triangle_contact_time',0x10025f50,0x100262fd),('complete_sse_normalize',0x100a7f30,0x100a7f9d),('actual_search_contact_time_call',0x10029da0,0x10029da5)])
    require(image.word(0x100c5300)==0x6753c21c and image.word(0x100d7908)==0x34000000,'Contact sentinel/epsilon constants')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    sources=['source_physics_triangle_contact_time_candidate.h','physics_triangle_contact_time_test_fixture.h','probe_physics_triangle_contact_time.cpp','probe_physics_triangle_contact_time_capture.cpp','test_physics_triangle_contact_time.cpp','build_physics_triangle_contact_time.cmd','verify_physics_triangle_contact_time.py','source_physics_line_simplex_candidate.h','native_vphysics_library_fixture.h','verify_ifm_draw_packet.py']
    result=dict(status='PASS',cases=len(rows),controlled_cases=controlled,captured_real_calls=len(captured),differences=0,choices=dict(collections.Counter(str(x[1]) if x[2] else 'sentinel' for x in reference)),safety=safety.stdout+safety.stderr,invalid_fixture_checks=invalidChecks,executables=executables,logs=logs,models=models,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),static_evidence=evidence,query_sha256=sha(inputs),source_sha256={name:sha(ROOT/'work'/name) for name in sources},limits=['Complete original25F50 and A7F30 execute intact; one original CALL29DA0 observed in process memory only; original trace answers unchanged','Owned uses original float arithmetic ordering and default53bit x87 final divide/add; nondefault floating controls and aliasing unverified','Unused count/vertex3/metadata bytes retained as opaque input, not geometric data','This is a contact-time dependency, not full296B0 search, penetration refinement28F40, normal generation24ED0 or editor integration','No editor rebuild, original writes, UI/Blender launch or dependencies installed'])
    result['mixed_precision_sensitive_cases']=sum(x[3] for x in reference)
    result['limits'][0]='Complete original25F50/A7F30 execute intact in direct native tests. CALL29DA0 instrumented but NOT reached by any of the 7,381 benchmark point-ray traces; real caller-path behavior for this helper remains UNVERIFIED. Original traces unchanged.'
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({key:result[key] for key in ['status','cases','controlled_cases','captured_real_calls','differences','choices','mixed_precision_sensitive_cases','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
