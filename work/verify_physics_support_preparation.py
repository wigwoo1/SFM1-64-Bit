"""Original complete packed-support preparation vs owned AMD64 and independent float rules."""
import collections,hashlib,json,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-support-preparation-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def frame(data):return struct.pack('<I',len(data))+data
def take(data,at):
    require(at+4<=len(data),'Missing frame length');n=struct.unpack_from('<I',data,at)[0];require(at+4+n<=len(data),'Truncated frame');return data[at+4:at+4+n],at+4+n
def split(data):
    result=[];at=0
    while at<len(data):row,at=take(data,at);result.append(row)
    return result
def encode(points,matrix,direction,count,first=0,runs=(),flags=2,present=True,blocked=False,old_groups=0,storage=None):
    capacity=max(old_groups*4,(count+3)//4*4) if storage is None else len(storage)
    storage=storage if storage is not None else [(91,92,93)]*capacity
    return struct.pack('<9I15f',int(present),int(blocked),flags,count,first,len(runs),len(points),old_groups,capacity,*matrix,*direction)+bytes(runs)+b''.join(struct.pack('<4f',*p) for p in points)+b''.join(struct.pack('<3f',*p) for p in storage)
def expected(wire):
    present,blocked,flags,count,first,nruns,npoints,oldgroups,capacity=struct.unpack_from('<9I',wire);matrix=struct.unpack_from('<12f',wire,36);direction=struct.unpack_from('<3f',wire,84);runs=list(wire[96:96+nruns]);at=96+nruns;points=[struct.unpack_from('<4f',wire,at+j*16) for j in range(npoints)];at+=16*npoints;storage=bytearray(wire[at:]);built=present and flags&6 and not blocked
    if not built:return struct.pack('<II',0,oldgroups)+storage+struct.pack('<I3f',0xffffffff,123456,123456,123456)
    if flags&4:
        indices=[];cursor=first
        for j in range(0,len(runs),2):
            length=runs[j] if j==0 else runs[j]
            indices.extend(range(cursor,cursor+length));cursor+=length
            if j+1<len(runs):cursor+=runs[j+1]
        require(len(indices)==count,'Independent run metadata count')
    else:indices=list(range(first,first+count))
    groups=(count+3)//4;indices+=indices[-1:]*(groups*4-len(indices));mul=lambda a,b:f32(a*b);add=lambda a,b:f32(a+b)
    for lane,index in enumerate(indices):
        p=points[index];v=[add(add(mul(p[0],matrix[j*4]),mul(p[1],matrix[j*4+1])),mul(p[2],matrix[j*4+2])) for j in range(3)];struct.pack_into('<3f',storage,lane*12,*v)
    from verify_physics_support import encode as encodeSupport,expected as select
    chosen=select(encodeSupport(2,[struct.unpack_from('<3f',storage,j*12) for j in range(groups*4)],direction))
    return struct.pack('<II',1,groups)+storage+chosen

def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS','Prior trace provenance');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        source=Path(e['source']);require(sha(source)==e['sha256'],'Original library changed');shutil.copyfile(source,libraries/source.name)
    executables=[]
    for name,machine in [('simplex_native_x86',0x14c),('simplex_capture_x86',0x14c),('simplex_owned_x64',0x8664),('simplex_safety_x64',0x8664)]:
        p=OUT/(name+'.exe');b=p.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==machine and b'asInvoker' in b,'Build architecture/manifest');executables.append(dict(path=str(p),sha256=sha(p),machine=hex(machine)))
    captured=[];observed=[];models=[];modeCounts=collections.Counter()
    for index,m in enumerate(prior['models']):
        case=run/('%03d'%index);case.mkdir();queries=Path(m['path'])/'queries.bin';old=Path(m['path'])/'native-traces.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(queries)==m['query_sha256'] and sha(old)==m['trace_sha256'],'Prior model/query/trace changed');trace=case/'traces.bin';record=case/'captures.bin'
        p=subprocess.run([str(OUT/'simplex_capture_x86.exe'),str(libraries),m['phy'],str(queries),str(trace),str(record)],capture_output=True,text=True,timeout=45,cwd=case);(case/'capture.log').write_text(p.stdout+p.stderr);require(p.returncode==0,'Support capture failed '+str(case)+' '+p.stdout+p.stderr);require(trace.read_bytes()==old.read_bytes(),'Support observation changed full original trace');raw=record.read_bytes();at=0;count=0
        while at<len(raw):
            query=struct.unpack_from('<I',raw,at)[0];at+=4;require(query<m['queries'],'Captured query index');row,at=take(raw,at);result,at=take(raw,at);captured.append(row);observed.append(result);count+=1;modeCounts['refused' if not struct.unpack_from('<I',result)[0] else ('rle' if struct.unpack_from('<I',row,8)[0]&4 else 'span')]+=1
        models.append(dict(model=m['model'],path=str(case),queries=m['queries'],support_calls=count,captures_sha256=sha(record),traces_sha256=sha(trace)));print('PASS observed',index+1,'calls='+str(count),p.stdout.strip(),flush=True)
    require(captured,'No real support calls');rows=[];rng=random.Random(0x26d40)
    identity=[1,0,0,0,0,1,0,0,0,0,1,0]
    for rle in [False,True]:
        for j in range(4096):
            count=rng.randint(1,64);first=rng.randint(0,16);lengths=[];left=count
            for part in range(rng.randint(1,min(5,count))-1):
                length=rng.randint(1,max(1,left-(4-part))) if left>4-part else 1
                length=min(length,left-1)
                if length<1:break
                lengths.append(length);left-=length
            lengths.append(left);runs=[];rawcount=first
            for part,length in enumerate(lengths):
                if part:runs.append(rng.randint(0,8));rawcount+=runs[-1]
                runs.append(length);rawcount+=length
            if not rle:runs=[];rawcount=first+count
            scale=rng.choice([1e-6,1,128]);points=[[f32(rng.randint(-4,4)*scale) for k in range(3)]+[float('nan')] for _ in range(rawcount+3)];matrix=[f32(rng.randint(-4,4)/4) if k%4!=3 else float('nan') for k in range(12)];direction=[f32(rng.randint(-3,3)) for _ in range(3)];oldgroups=rng.randint(0,20);rows.append(encode(points,matrix,direction,count,first,runs,6 if rle and j%2 else (4 if rle else 2),old_groups=oldgroups))
    for count in range(1,33):
        for rle in [False,True]:
            points=[(float(j),-0.,float(-j),float('inf')) for j in range(count+1)]
            rows.append(encode(points,identity,(0.,-0.,0.),count,runs=[count] if rle else [],flags=4 if rle else 2))
    for present in [False,True]:
        for blocked in [False,True]:
            for flags in range(8):
                if present and flags&6 and not blocked:continue
                rows.append(encode([], [float('nan')]*12,(float('nan'),0,0),0,flags=flags,present=present,blocked=blocked,old_groups=1,storage=[(float('nan'),-0.,float('inf'))]*4))
    controlled=len(rows);rows+=captured;query=run/'queries.bin';query.write_bytes(b''.join(frame(row) for row in rows));reference=[expected(row) for row in rows];(run/'independent_expected.bin').write_bytes(b''.join(frame(row) for row in reference));outputs=[];logs=[]
    for name in ['simplex_native_x86','simplex_owned_x64']:
        dest=run/(name+'.bin');p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(query),str(dest)],capture_output=True,text=True,timeout=90);logs.append(p.stdout+p.stderr);require(p.returncode==0,'Direct support failed '+name+' '+logs[-1]);data=split(dest.read_bytes());require(len(data)==len(rows),'Support output extent');outputs.append(data)
    differences=[]
    for j,row in enumerate(rows):
        native,owned=outputs[0][j],outputs[1][j];actual=observed[j-controlled] if j>=controlled else native
        if not native==owned==reference[j]==actual:
            differences.append(dict(case=j,flags=struct.unpack_from('<I',row,8)[0],captured=j>=controlled,input=row.hex(),native=native.hex(),owned=owned.hex(),expected=reference[j].hex(),observed=actual.hex()))
            if len(differences)==8:break
    (run/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Support mismatch '+str(run/'comparison.json'))
    safety=subprocess.run([str(OUT/'simplex_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Safety '+safety.stdout+safety.stderr)
    invalid=run/'invalid';invalid.mkdir();bad=[('empty',b'',True),('truncated_length',b'\x01\x00',True),('truncated_payload',frame(rows[0])[:-1],True)]
    for label,at,bits in [('bad_present',0,2),('zero_vertex_count',12,0),('bad_run_extent',20,15),('nan_matrix',36,0x7fc00000)]:
        row=bytearray(rows[0]);struct.pack_into('<I',row,at,bits);bad.append((label,frame(row),label in ['bad_present','bad_run_extent']))
    row=bytearray(rows[0]);struct.pack_into('<f',row,36,3e38);bad.append(('arithmetic_overflow',frame(row),False));invalidChecks=0
    for label,data,both in bad:
        path=invalid/(label+'.bin');path.write_bytes(data)
        for name in ['simplex_owned_x64']+(['simplex_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(path),str(invalid/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Unsafe malformed fixture handling '+label+' '+name);invalidChecks+=1
    from verify_ifm_draw_packet import check_ranges
    image,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_preparation_dispatch_span',0x10026a10,0x10026bb3),('complete_compressed_run_preparation',0x100266f0,0x10026a02),('complete_packed_rotation',0x100a8dc0,0x100a8efa),('complete_packed_selection',0x10026bc0,0x10026d37),('actual_preparation_call',0x1002953b,0x10029540)])

    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy library changed')
    for m in prior['models']:
        for field in ['phy','mdl']:require(sha(Path(m[field]))==m[field+'_sha256'],'Original asset changed')
    receipt=dict(status='PASS',cases=len(rows),controlled_cases=controlled,captured_real_calls=len(captured),differences=0,captured_modes=dict(modeCounts),all_modes=dict(collections.Counter('refused' if not struct.unpack_from('<I',expected(row))[0] else ('rle' if struct.unpack_from('<I',row,8)[0]&4 else 'span') for row in rows)),safety=safety.stdout+safety.stderr,executables=executables,logs=logs,models=models,original_trace_cases=prior['totals']['traces'],all_original_trace_bytes_unchanged=True,libraries=prior['libraries'],current_library_copies=str(libraries),prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),query_sha256=sha(query),static_evidence=evidence,source_sha256={name:sha(ROOT/'work'/name) for name in ['source_physics_support_preparation_candidate.h','physics_support_preparation_test_fixture.h','native_physics_support_preparation_fixture.h','probe_physics_support_preparation.cpp','probe_physics_support_preparation_capture.cpp','test_physics_support_preparation.cpp','build_physics_support_preparation.cmd','verify_physics_support_preparation.py','source_physics_support_candidate.h','physics_support_test_fixture.h','native_physics_support_fixture.h','native_vphysics_library_fixture.h','verify_physics_support.py']},limits=['Complete original26A10/266F0/A8DC0 execute intact and native26D40 packed selection follows; owned preparation composes with verified owned selector','Real captures use actual original raw ledge vertices,24-byte cache metadata,prepared transform and old storage; raw PHY/runtime metadata generation/binding still not reconstructed','Only original CALL2953B instrumented in process memory; bridge calls real26A10, then pure original packed support selection for canonical direction; full original TraceBox unchanged','Cache storage tails and refusal states preserved byte-for-byte; float padding and translation columns intentionally unused; malformed run metadata refused only by owned safety guards','No complete owned GJK/compact trace/normal/startsolid/filter/registry/editor integration proof; default floating controls/noaliasdomain only; no original writes/editorbuild/UI/dependencies/idlehelpers'])
    receipt['invalid_fixture_checks']=invalidChecks
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({key:receipt[key] for key in ['status','cases','controlled_cases','captured_real_calls','differences','captured_modes','all_modes','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
