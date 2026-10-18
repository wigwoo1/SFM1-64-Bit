"""Compare owned x64 PHY GetAABB against unchanged original vphysics.
All scene poses/queries are inputs; no native intermediate state seeds x64.
"""
import collections,hashlib,json,random,re,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/physics-aabb-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
DIRECT=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
PRIOR=ROOT/'outputs/static-prop-partition-registration-20261016/verification-or1d0we6/results.json'
REGISTRY=ROOT/'outputs/static-prop-point-registry-20261016/verification-72v49r60/results.json'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def main():
 prior=json.loads(PRIOR.read_text());direct=json.loads(DIRECT.read_text());require(prior['status']==direct['status']=='PASS','Prior proof status')
 for p,s in prior['original_hashes'].items():require(sha(Path(p))==s,'Original changed '+p)
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
 for name in ['tier0.dll','vphysics.dll']:shutil.copyfile(GAME/'bin'/name,libs/name)
 image,evidence=check_ranges(GAME/'bin/vphysics.dll','vphysics-disassembly.txt',[('entry',0x10005e20,0x10005e2c),('whole_GetAABB',0x1002a4f0,0x1002a6b5),('tree_bounds',0x10029f80,0x1002a07e),('support_constructor',0x1002a120,0x1002a331),('bind_ledge',0x100294f0,0x10029559)])
 constants={hex(a):hex(image.word(a)) for a in [0x100ee008,0x100ee00c,0x100d7920,0x100d791c]};constants['runtime_zero_origin_gate']='BSS100FA3CC/D0/D4; native probe checks each normally initialized word is0';(run/'original-code-evidence.json').write_text(json.dumps(dict(ranges=evidence,constants=constants),indent=2))
 executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664),('safety_x64',0x8664)]:
  exe=OUT/('phy_aabb_'+name+'.exe');raw=exe.read_bytes();p=struct.unpack_from('<I',raw,60)[0];require(raw[p:p+4]==b'PE\0\0' and struct.unpack_from('<H',raw,p+4)[0]==machine and b'asInvoker' in raw,'Executable identity');executables.append(dict(path=str(exe),sha256=sha(exe),machine=hex(machine)))
 def execute(name,args,log):
  p=subprocess.run([str(OUT/('phy_aabb_'+name+'.exe')),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=60);log.write_text(p.stdout+p.stderr);require(p.returncode==0,'Bounds probe failed '+str(log)+' '+p.stdout+p.stderr);return p.stdout
 # Reuse only pose inputs and frozen final bounds from original registration
 # proof. Native intermediate state is neither read nor passed to owned code.
 registration=[]
 for entry in prior['records']:
  p=Path(entry['path']);q=(p/'queries.bin').read_bytes();a=(p/'native.bin').read_bytes();require(sha(p/'native.bin')==entry['sha256'],'Prior registration data changed')
  for j in range(len(q)//60):
   if struct.unpack_from('<I',a,j*56+20)[0]:registration.append((entry['model'],q[j*60:j*60+24],a[j*56+32:(j+1)*56]))
 byModel=collections.defaultdict(list)
 for name,q,a in registration:byModel[name].append((q,a))
 rng=random.Random(2026101647);records=[];totals=collections.Counter();metadata=collections.Counter()
 for index,model in enumerate(direct['models']):
  phy=Path(model['phy']);require(sha(phy)==model['phy_sha256'],'PHY identity');case=run/('%03d'%index);case.mkdir();queries=[];qbytes=[]
  def add(origin,angles,label):qbytes.append(struct.pack('<6f',*origin,*angles));queries.append(label)
  # Signed zero, zero-origin packed search, nonzero-origin span/walk search,
  # cardinal/large Euler angles and tiny origin values cover arithmetic gates.
  angles=[(0,0,0),(0,90,0),(90,0,0),(0,0,90),(180,180,180),(-90,-180,270),(360,-360,720),(35999,-35999,36000),(23,47,99)]
  origins=[(0,0,0),(-0.,0,-0.),(128,-256,512),(1.234,-9.876,3.14),(1e-30,0,0),(0,-1e-30,0),(0,0,1e-30),(-10000,20000,-30000)]
  for origin in origins:
   for angle in angles:add(origin,angle,'directed')
  for j in range(256):add((0,0,0) if j%4==0 else [rng.uniform(-30000,30000) for _ in range(3)],[rng.uniform(-36000,36000) for _ in range(3)],'random')
  registrationStart=len(qbytes)
  for q,_ in byModel[model['model']]:qbytes.append(q);queries.append('registration')
  query=case/'queries.bin';query.write_bytes(b''.join(qbytes));a=case/'native.bin';b=case/'owned.bin';execute('native_x86',[libs,phy,query,a],case/'native.log');execute('owned_x64',[libs,phy,query,b],case/'owned.log');native=a.read_bytes();owned=b.read_bytes();require(len(native)==len(qbytes)*24 and len(owned)==len(native),'Bounds result extent')
  if native!=owned:
   first=next(i for i,(x,y) in enumerate(zip(native,owned)) if x!=y);j=first//24;diff=dict(model=model['model'],case=j,label=queries[j],pose=struct.unpack('<6f',qbytes[j]),native=struct.unpack_from('<6I',native,j*24),owned=struct.unpack_from('<6I',owned,j*24));(run/'difference.json').write_text(json.dumps(diff,indent=2));raise RuntimeError('Bounds mismatch '+str(run/'difference.json'))
  for j,(_,expected) in enumerate(byModel[model['model']]):require(native[(registrationStart+j)*24:(registrationStart+j+1)*24]==expected,'Registration bounds changed')
  safety=execute('safety_x64',[phy],case/'safety.log');m=re.search(r'checks=(\d+) metadata_seeded=(\d+) metadata_span=(\d+) metadata_runs=(\d+) no_metadata=(\d+)',safety);require(m is not None,'Safety transcript');totals['safety_checks']+=int(m[1]);metadata.update(dict(zip(['seeded','span','runs','no_metadata'],map(int,m.groups()[1:]))));totals.update(queries);totals['queries']+=len(qbytes)
  records.append(dict(model=model['model'],phy=str(phy),phy_sha256=sha(phy),path=str(case),queries=len(qbytes),query_sha256=sha(query),bounds_sha256=sha(b),safety=safety.strip()))
  if (index+1)%10==0:print('PASS exact native/x64 physics bounds models',index+1,flush=True)
 require(len(records)==61 and totals['registration']==778,'Corpus completeness');require(metadata['span']>0 and metadata['runs']>0 and metadata['seeded']>0,'Support metadata branch coverage')
 old=json.loads(REGISTRY.read_text());require(old['status']=='PASS','Prior clipping proof');previous=Path(old['path']);require(sha(previous/'queries.bin')==old['query_sha256'] and sha(previous/'native-ordered.bin')==old['trace_sha256'],'Clipping reference identity');regression=run/'regression';regression.mkdir();bsp=GAME/'tf_movies/maps/mtt_soldier.bsp';mounts=previous/'mounts.txt';output=regression/'owned.bin';regressionExecutables=[]
 for name,args in [('prop_registry_safety_x64',[bsp,mounts]),('prop_registry_owned_x64',[bsp,mounts,previous/'queries.bin',output])]:
  exe=OUT/'regression'/(name+'.exe');r=subprocess.run([str(exe),*map(str,args)],cwd=run,capture_output=True,text=True,timeout=60);(regression/(name+'.log')).write_text(r.stdout+r.stderr);require(r.returncode==0,'Clipping regression failed '+r.stdout+r.stderr);regressionExecutables.append(dict(path=str(exe),sha256=sha(exe),machine='0x8664'));require(name!='prop_registry_safety_x64' or r.stdout.strip()==old['safety'],'Clipping safety checks changed')
 require(sha(output)==old['trace_sha256'] and sha(Path(str(output)+'.bindings'))==old['binding_sha256'] and Path(str(output)+'.events').read_bytes()==(previous/'native-ordered.events').read_bytes(),'Existing clipping trace/bindings/events changed');regressionResult=dict(queries=old['queries'],safety=old['safety'],trace_differences=0,binding_differences=0,event_differences=0,path=str(regression),reference_receipt=str(REGISTRY),reference_receipt_sha256=sha(REGISTRY),executables=regressionExecutables);print('PASS existing prop clipping regression18430queries/994safety',flush=True)
 for p,s in prior['original_hashes'].items():require(sha(Path(p))==s,'Original changed after tests '+p)
 sources={str(ROOT/'work'/p):sha(ROOT/'work'/p) for p in ['source_phy_aabb_candidate.h','source_phy_point_trace_candidate.h','probe_phy_aabb.cpp','test_phy_aabb.cpp','build_phy_aabb.cmd','verify_phy_aabb.py']}
 result=dict(status='PASS',path=str(run),models=len(records),totals=dict(totals),metadata_convexes=dict(metadata),bounds_differences=0,registration_bounds_differences=0,executables=executables,sources=sources,records=records,clipping_regression=regressionResult,original_hashes=prior['original_hashes'],registration_receipt=str(PRIOR),registration_receipt_sha256=sha(PRIOR),limitations=['Checked one-solid compact PHY scope, same scope as owned point tracer; no original DLL dependency in owned x64 code.','GetAABB new method shares existing immutable geometry/metadata. No spatial tree insertion/enumeration or runtime registration integration yet.','Native original physics code is unchanged and normally initialized from isolated workspace DLL copies; test-only x86 reference, not production dependency.','No editor build/GUI, full-scene collision claim or rendering/performance improvement claim.'])
 (run/'results.json').write_text(json.dumps(result,indent=2));print('PASS '+str(run/'results.json'),flush=True)
if __name__=='__main__':main()
