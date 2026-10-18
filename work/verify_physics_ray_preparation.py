"""Owned point-ray inputs vs complete original preparation and real TraceBox caller."""
import hashlib,json,math,random,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-ray-preparation-20261015'
PRIOR=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def require(pass_,message):
    if not pass_:raise RuntimeError(message)
def main():
    prior=json.loads(PRIOR.read_text());require(prior['status']=='PASS' and prior['mesh_differences']==0,'Prior geometry receipt');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libraries=run/'libraries';libraries.mkdir()
    for e in prior['libraries']:
        src=Path(e['source']);require(sha(src)==e['sha256'],'Pinned original library');shutil.copyfile(src,libraries/src.name);require(sha(libraries/src.name)==e['sha256'],'Library copy')
    exeEvidence=[]
    for name,machine in [('preparation_native_x86',0x14c),('preparation_owned_x64',0x8664),('preparation_safety_x64',0x8664)]:
        exe=OUT/(name+'.exe');b=exe.read_bytes();p=struct.unpack_from('<I',b,60)[0];require(b[p:p+4]==b'PE\0\0' and struct.unpack_from('<H',b,p+4)[0]==machine and b'asInvoker' in b,'Architecture/manifest');exeEvidence.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
    groups=[]
    for i,m in enumerate(prior['models']):
        p=Path(m['path'])/'queries.bin';require(sha(Path(m['phy']))==m['phy_sha256'] and sha(p)==m['query_sha256'],'Prior original PHY/query hash');groups.append((m['model'],Path(m['phy']),p.read_bytes(),Path(m['path'])/'native-traces.bin',m['trace_sha256']))
    rows=[]
    def add(start=(0,0,0),delta=(3,4,0),origin=(0,0,0),angles=(0,0,0),mask=0x4481):rows.append(struct.pack('<12fI',*start,*delta,*origin,*angles,mask))
    add(start=(10,20,30),origin=(1,2,3));add(delta=(0,0,0));add(start=(-0.,0,-0.),delta=(0,-0.,0),origin=(-0.,0,0),angles=(-0.,0,-0.))
    for axis in range(3):
        for length in [-.001,-.0004,-.00039,-.0001,-1e-6,1e-6,.0001,.00039,.0004,.001]:
            delta=[0.,0.,0.];delta[axis]=length;add(delta=delta)
    rng=random.Random(2026101518)
    for i in range(8192):
        add(start=[rng.uniform(-65536,65536) for _ in range(3)],delta=([0,0,0] if i%17==0 else [rng.uniform(-8192,8192) for _ in range(3)]),origin=[rng.uniform(-65536,65536) for _ in range(3)],angles=[rng.uniform(-36000,36000) for _ in range(3)],mask=rng.getrandbits(32))
    groups.append(('controlled vectors/angles/thresholds',groups[0][1],b''.join(rows),None,None));records=[];total=0
    for index,(name,phy,raw,oldTrace,oldHash) in enumerate(groups):
        case=run/('%03d'%index);case.mkdir();query=case/'queries.bin';query.write_bytes(raw);count=len(raw)//52;require(count*52==len(raw),'Query extent');outputs=[];logs=[]
        for exe in ['preparation_native_x86','preparation_owned_x64']:
            dest=case/(exe+'.bin');p=subprocess.run([str(OUT/(exe+'.exe')),str(libraries),str(phy),str(query),str(dest)],capture_output=True,text=True,timeout=45,cwd=case);logs.append(p.stdout+p.stderr);(case/(exe+'.log')).write_text(logs[-1]);require(p.returncode==0,'Preparation probe failed '+str(case)+' '+logs[-1]);data=dest.read_bytes();require(len(data)==count*168,'Preparation result extent');outputs.append(data)
        differences=[]
        for n in range(count):
            a,b=(x[n*168:(n+1)*168] for x in outputs)
            if a!=b:
                offsets=[i for i in range(42) if a[i*4:(i+1)*4]!=b[i*4:(i+1)*4]];differences.append(dict(case=n,fields=offsets,native=struct.unpack('<42f',a),owned=struct.unpack('<42f',b)))
                if len(differences)==8:break
        (case/'comparison.json').write_text(json.dumps(dict(differences=differences),indent=2)+'\n');require(not differences,'Preparation mismatch '+str(case/'comparison.json'))
        trace=case/'preparation_native_x86.bin.traces.bin';traceBytes=trace.read_bytes();require(len(traceBytes)==count*60,'Full physics trace extent')
        for n in range(count):
            fraction=struct.unpack_from('<f',traceBytes,n*60+44)[0];require(math.isfinite(fraction) and 0<=fraction<=1,'Full physics trace fraction')
        baseline=None
        if oldTrace:
            require(sha(oldTrace)==oldHash and traceBytes==oldTrace.read_bytes(),'Observed preparation bridge changed original trace results');baseline=dict(path=str(oldTrace),sha256=oldHash)
        records.append(dict(name=name,phy=str(phy),phy_sha256=sha(phy),cases=count,query_sha256=sha(query),preparation_sha256=hashlib.sha256(outputs[0]).hexdigest(),trace_sha256=sha(trace),baseline=baseline,logs=logs,path=str(case)));total+=count;print('PASS',index+1,name,'cases='+str(count),flush=True)
    safety=subprocess.run([str(OUT/'preparation_safety_x64.exe')],capture_output=True,text=True,timeout=15);require(safety.returncode==0,'Safety test '+safety.stdout+safety.stderr)
    invalid=run/'invalid';invalid.mkdir();wire=bytearray(rows[0]);bad=[]
    for label,offset,value in [('nan_start',0,0x7fc00000),('inf_delta',12,0x7f800000),('inf_origin',24,0x7f800000),('nan_angle',36,0x7fc00000)]:
        b=bytearray(wire);struct.pack_into('<I',b,offset,value);bad.append((label,bytes(b),True))
    b=bytearray(wire);struct.pack_into('<f',b,36,36001.);bad.append(('angle_range',bytes(b),True));bad.append(('truncated',bytes(wire[:51]),True))
    b=bytearray(wire);struct.pack_into('<3f',b,12,1e30,0,0);bad.append(('local_squared_overflow',bytes(b),False))
    invalidCount=0
    for label,b,both in bad:
        q=invalid/(label+'.bin');q.write_bytes(b)
        for name in ['preparation_owned_x64']+(['preparation_native_x86'] if both else []):
            p=subprocess.run([str(OUT/(name+'.exe')),str(libraries),str(groups[0][1]),str(q),str(invalid/(label+'-'+name+'.out'))],capture_output=True,text=True,timeout=15);require(p.returncode==1 and 'fault' not in p.stderr,'Invalid fixture did not reject normally '+label);invalidCount+=1
    from verify_ifm_draw_packet import check_ranges
    physics,evidence=check_ranges(Path(prior['libraries'][1]['source']),'vphysics-disassembly.txt',[('complete_ray_constructor',0x10027310,0x100273f0),('complete_ray_prepare',0x10027400,0x10027571),('complete_collector_constructor',0x100295f0,0x100296a6),('complete_motion_constructor',0x1002a120,0x1002a331),('complete_IVP_orientation',0x10001120,0x100011c1),('complete_AngleVectors',0x100a8540,0x100a86b1),('complete_inverse_point',0x1006f220,0x1006f2c0),('complete_inverse_vector',0x1006f400,0x1006f48d),('complete_direction_normalize',0x1006df70,0x1006dfe6),('full_TraceBox_caller',0x1002b260,0x1002b561)])
    constants={0x100ee008:0x3cd013a9,0x100ee014:0x3d000000,0x100c6864:0x2edbe6ff,0x100c744c:0x322bcc77,0x100c5dc4:0x3c8efa35}
    require(all(physics.word(a)==v for a,v in constants.items()),'Original physics preparation constants')
    for e in prior['libraries']:require(sha(Path(e['source']))==e['sha256'] and sha(libraries/Path(e['source']).name)==e['sha256'],'Original/copy library changed')
    receipt=dict(status='PASS',scope='Owned point-ray preparation, not owned GJK/physics tracing',cases=total,bytes_per_result=168,differences=0,benchmark_cases=prior['totals']['traces'],controlled_cases=len(rows),all_61_original_trace_baselines_unchanged=True,safety=safety.stdout+safety.stderr,invalid_fixture_checks=invalidCount,executables=exeEvidence,prior_receipt=str(PRIOR),prior_receipt_sha256=sha(PRIOR),libraries=prior['libraries'],current_copy_directory=str(libraries),groups=records,static_evidence=evidence,source_sha256={n:sha(ROOT/'work'/n) for n in ['source_physics_ray_preparation_candidate.h','native_vphysics_library_fixture.h','probe_physics_ray_preparation.cpp','test_physics_ray_preparation.cpp','build_physics_ray_preparation.cmd','verify_physics_ray_preparation.py','angle_basis_candidate.h','angle_basis_products_candidate.h','x64_fsin_cos.asm']},limits=['Original ray/motion/collector/prepare functions execute intact with an actual loaded PHY model; direct normalized state must equal observed actual full TraceBox input','Test-only loaded-copy call bridge atRVA2B459 calls original27400 then observes results before original compact/GJK trace proceeds; other loaded executable bytes verified unchanged','Owned x64 code computes 42 preparation floats; complete 7381 prior original trace results remain unchanged, but owned intersections are not implemented','Point rays only; original ray descriptor contents-mask callback null; actual engine solid6/filter/partition/material integration unverified','Existing verified x64 FSINCOS helper reused; finite angles bounded+-36000; unsupported/nonfinite/overflow inputs refused','No original writes, editor source changes/build, UI applications, installs or idle helper processes'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({k:receipt[k] for k in ['status','cases','benchmark_cases','controlled_cases','differences','safety','invalid_fixture_checks']}));print('receipt='+str(run/'results.json'))
if __name__=='__main__':main()
