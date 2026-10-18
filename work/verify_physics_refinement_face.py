"""Complete original25460/25B10 vs owned AMD64 and real call captures."""
import collections,hashlib,json,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-refinement-face-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def encode(points,direction,mode,rng):
    row=bytearray(rng.getrandbits(8) for _ in range(96));struct.pack_into('<I',row,0,mode)
    for j,p in enumerate(points):struct.pack_into('<3f',row,4+j*16,*p)
    struct.pack_into('<3f',row,72,*direction);return bytes(row)
def expected(row):
    mode=struct.unpack_from('<I',row)[0];points=[struct.unpack_from('<3f',row,4+j*16) for j in range(4)];direction=struct.unpack_from('<3f',row,72)
    sub=lambda a,b:f32(a-b);mul=lambda a,b:f32(a*b);add=lambda a,b:f32(a+b)
    def offset(a,b):return [sub(a[j],b[j]) for j in range(3)]
    def cross(a,b):return [sub(mul(a[1],b[2]),mul(a[2],b[1])),sub(mul(a[2],b[0]),mul(a[0],b[2])),sub(mul(a[0],b[1]),mul(a[1],b[0]))]
    def dot(a,b):return add(add(mul(a[0],b[0]),mul(a[1],b[1])),mul(a[2],b[2]))
    a,b,c,d=points;u,v,w=[offset(p,d) for p in [a,b,c]];normals=[cross(u,v),cross(v,w),cross(w,u)]
    if mode:normals.append(cross(offset(c,a),offset(b,a)))
    sentinel=struct.unpack('<f',struct.pack('<I',0x6753c21c))[0];times=[]
    for j,n in enumerate(normals):
        denominator=dot(direction,n);times.append(f32(dot(a if j==3 else d,n)/denominator) if denominator>0 else sentinel)
    # Original priority is base3, then0,1,2; update priority2, then0,1.
    chosen=3 if mode else 2
    for j in range(3 if mode else 2):
        if times[chosen]>times[j]:chosen=j
    slots=[row[4+j*16:20+j*16] for j in range(4)]
    if chosen==3:slots=[slots[0],slots[2],slots[1],slots[1]]
    else:slots[[2,0,1][chosen]]=slots[3]
    return b''.join(slots)+struct.pack('<I3f',3,*normals[chosen]),str(mode)+':'+str(chosen)
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior trace status');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        source=Path(e['source']);require(sha(source)==e['sha256'],'Original library changed');shutil.copyfile(source,libraries/source.name)
    executables=[]
    for name,machine in [('face_native_x86',0x14c),('face_capture_x86',0x14c),('face_owned_x64',0x8664),('face_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Build architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    captured=[];observed=[];models=[];capturedModes=collections.Counter()
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior asset/query/trace changed');trace=case/'traces.bin';record=case/'captures.bin'
        p=subprocess.run([str(OUT/'face_capture_x86.exe'),str(libraries),m['phy'],str(queries),str(trace),str(record)],capture_output=True,text=True,timeout=45,cwd=case);(case/'capture.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Face capture failed '+str(case)+' '+p.stdout+p.stderr);require(trace.read_bytes()==old.read_bytes(),'Face observation changed original trace');raw=record.read_bytes();require(len(raw)%180==0,'Face capture extent')
        for at in range(0,len(raw),180):
            require(struct.unpack_from('<I',raw,at)[0]<m['queries'],'Captured query index');row=raw[at+4:at+100];captured.append(row);observed.append(raw[at+100:at+180]);capturedModes[struct.unpack_from('<I',row)[0]]+=1
        models.append(dict(model=m['model'],path=str(case),calls=len(raw)//180,captures_sha256=sha(record),traces_sha256=sha(trace)));print('PASS original',index+1,'calls='+str(len(raw)//180),flush=True)
    rng=random.Random(0x2546025b10);rows=[]
    for mode in [0,1]:
        for j in range(16384):
            scale=rng.choice([2**-12,1,128]);points=[[f32(rng.randint(-8,8)*scale) for _ in range(3)] for _ in range(4)]
            if j%11==0:points[3]=points[0][:]
            if j%17==0:points=[[float(x)*scale,0.,0.] for x in [-1,0,1,2]]
            if j%29==0:points=[[rng.choice([-0.,0.]) for _ in range(3)] for _ in range(4)]
            direction=[f32(rng.randint(-4,4)*rng.choice([2**-20,1,128])) for _ in range(3)];rows.append(encode(points,direction,mode,rng))
        tetra=[(1.,0.,0.),(0.,1.,0.),(0.,0.,1.),(0.,0.,0.)]
        for direction in [(0.,0.,0.),(-0.,0.,-0.),(1.,0.,0.),(0.,1.,0.),(0.,0.,1.),(-1.,-1.,-1.),(1.,1.,1.)]:rows.append(encode(tetra,direction,mode,rng))
    controlled=len(rows);rows+=captured;query=run/'queries.bin';query.write_bytes(b''.join(rows));reference=[expected(row) for row in rows];(run/'expected.bin').write_bytes(b''.join(x[0] for x in reference));outputs=[];logs=[]
    for name in ['face_native_x86','face_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(query),str(dest)],capture_output=True,text=True,timeout=60);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Face direct probe '+name+' '+logs[-1]);data=dest.read_bytes();require(len(data)==len(rows)*80,'Face output extent');outputs.append(data)
    differences=[]
    for j,row in enumerate(rows):
        a,b=outputs[0][j*80:(j+1)*80],outputs[1][j*80:(j+1)*80];c=reference[j][0];actual=observed[j-controlled] if j>=controlled else a
        if not a==b==c==actual:
            differences.append(dict(case=j,captured=j>=controlled,input=row.hex(),native=a.hex(),owned=b.hex(),expected=c.hex(),observed=actual.hex()))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Face mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'face_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Face safety '+safety.stdout+safety.stderr)
    invalid=run/'invalid';invalid.mkdir();bad=[('empty',b'',True),('truncated',rows[0][:-1],True)]
    for label,at,bits in [('bad_mode',0,2),('nan_point',4,0x7fc00000),('inf_direction',72,0x7f800000)]:
        row=bytearray(rows[0]);struct.pack_into('<I',row,at,bits);bad.append((label,bytes(row),True))
    row=bytearray(rows[0]);struct.pack_into('<3f',row,4,3e38,0,0);struct.pack_into('<3f',row,20,0,3e38,0);struct.pack_into('<3f',row,36,0,0,3e38);struct.pack_into('<3f',row,52,0,0,0);bad.append(('overflow',bytes(row),False));invalidChecks=0
    for label,data,both in bad:
        path=invalid/(label+'.bin');path.write_bytes(data)
        for name in ['face_owned_x64']+(['face_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(path),str(invalid/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Invalid fixture '+label+' '+name);invalidChecks+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_initial_four_face_choice',0x10025460,0x10025afe),('complete_three_face_update',0x10025b10,0x10025f4d),('actual_initial_face_call',0x10028f6b,0x10028f70),('actual_update_face_call',0x10029170,0x10029175)])
    require([image.word(0x10025b00+j*4) for j in range(4)]==[0x10025896,0x10025901,0x1002596b,0x100259d6],'Original face dispatch table');require(image.word(0x100c5300)==0x6753c21c,'Original sentinel')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy DLL changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    sources=['source_physics_refinement_face_candidate.h','physics_refinement_face_test_fixture.h','probe_physics_refinement_face.cpp','probe_physics_refinement_face_capture.cpp','test_physics_refinement_face.cpp','build_physics_refinement_face.cmd','verify_physics_refinement_face.py','source_physics_line_simplex_candidate.h','native_vphysics_library_fixture.h','verify_ifm_draw_packet.py']
    result=dict(status='PASS',cases=len(rows),controlled_cases=controlled,captured_real_calls=len(captured),captured_modes=dict(capturedModes),differences=0,choices=dict(collections.Counter(x[1] for x in reference)),safety=safety.stdout+safety.stderr,invalid_fixture_checks=invalidChecks,executables=executables,logs=logs,models=models,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),query_sha256=sha(query),static_evidence=evidence,source_sha256={name:sha(ROOT/'work'/name) for name in sources},limits=['Complete25460/25B10 execute intact in original runtime; only two CALL sites instrumented in loaded workspace copies; trace answers unchanged','All80 normalized output bytes compared, including complete68byte simplex, count and feature words/high bits; source input count and old normal bytes are opaque/ignored','Finite arithmetic domain, default floating controls and disjoint output/direction/simplex buffers only; aliasing/nonfinite original behavior unverified','Not complete28F40 refinement,296B0 search,24ED0 normal/witness generation or editor integration','No editor rebuild, renderer changes, original writes, UI/Blender launch or dependencies installed'])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({key:result[key] for key in ['status','cases','controlled_cases','captured_real_calls','captured_modes','differences','choices','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
