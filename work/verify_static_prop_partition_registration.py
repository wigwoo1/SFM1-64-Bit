"""Characterize original registration eligibility/mask/handle/bounds forwarding.
Test-only: no owned AABB math, real partition ordering, or editor integration.
"""
import collections,hashlib,json,random,shutil,struct,subprocess,tempfile
from pathlib import Path
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/static-prop-partition-registration-20261016'
GAME=Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game')
PRIOR=ROOT/'outputs/studio-vcollide-read-20261016/verification-gr1jjsch/results.json'
DIRECT=ROOT/'outputs/native-vphysics-prop-trace-20261015/verification-rin0e0ci/results.json'
PINS={'bin/engine.dll':'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
 'bin/tools/ifm.dll':'8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4',
 'tf_movies/maps/mtt_soldier.bsp':'60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea',
 'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx':'5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise RuntimeError(message)
def main():
 prior=json.loads(PRIOR.read_text());direct=json.loads(DIRECT.read_text());require(prior['status']==direct['status']=='PASS','Prior receipt')
 originals={**prior['original_hashes'],**{str(GAME/p):s for p,s in PINS.items()},**{e['source']:e['sha256'] for e in prior['libraries']}}
 for p,s in originals.items():require(sha(Path(p))==s,'Original changed '+p)
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));libs=run/'libraries';libs.mkdir()
 for e in prior['libraries']:
  if Path(e['source']).name in ['tier0.dll','vphysics.dll']:shutil.copyfile(e['source'],libs/Path(e['source']).name)
 _,code=check_ranges(GAME/'bin/engine.dll','engine-disassembly.txt',[('whole_prop_registration',0x101c4580,0x101c46d2),('collision_model_lookup',0x10147ee0,0x10147f38),('partition_getter',0x101bea40,0x101bea46)])
 _,physicsCode=check_ranges(GAME/'bin/vphysics.dll','vphysics-disassembly.txt',[('GetAABB_entry',0x10005e20,0x10005e2c),('whole_GetAABB',0x1002a4f0,0x1002a6b5)])
 (run/'original-code-evidence.json').write_text(json.dumps({'engine':code,'physics':physicsCode},indent=2))
 b=(GAME/'tf_movies/maps/mtt_soldier.bsp').read_bytes();g=struct.unpack_from('<I',b,568)[0]
 for i in range(struct.unpack_from('<I',b,g)[0]):
  tag,_,_,off,length=struct.unpack_from('<IHHII',b,g+4+i*16)
  if tag==0x73707270:break
 n=struct.unpack_from('<I',b,off)[0];names=[b[off+4+i*128:off+4+(i+1)*128].split(b'\0')[0].decode('ascii') for i in range(n)];p=off+4+n*128;p+=4+2*struct.unpack_from('<I',b,p)[0];count=struct.unpack_from('<I',b,p)[0];p+=4;props=[b[p+i*60:p+(i+1)*60] for i in range(count)];require((n,count)==(79,485),'Benchmark inventory')
 assets={r['model']:r for r in prior['records']};groups=[[] for _ in names];rows=[];rng=random.Random(2026101641)
 def add(m,origin,angles,mins,maxs,solid,cache,handle,label,ordinal=None):
  row=dict(model=m,solid=solid,cache=cache,handle=handle,label=label,ordinal=ordinal,origin=origin,mins=mins,maxs=maxs)
  wire=struct.pack('<12f3I',*origin,*angles,*mins,*maxs,solid,cache,handle);groups[m].append((len(rows),wire));rows.append(row)
 for i,r in enumerate(props):
  m=struct.unpack_from('<H',r,24)[0];mdl=Path(assets[names[m]]['mdl']).read_bytes();lo=struct.unpack_from('<3f',mdl,104);hi=struct.unpack_from('<3f',mdl,116);origin=struct.unpack_from('<3f',r);angles=struct.unpack_from('<3f',r,12);cache=0 if assets[names[m]]['phy'] else 1
  add(m,origin,angles,lo,hi,r[30],cache,i,'benchmark',i)
  # Actual no-collision and empty-vcollide failure branches, independent of
  # model's normal availability. Valid native data only, no malformed originals.
  for absent in [1,2]:add(m,origin,angles,lo,hi,6,absent,65534,'absent-or-empty')
  for state in [0,1,2]:add(m,origin,angles,lo,hi,0,state,65535,'nonsolid')
  if r[30]==6 and cache==0:
   # Changing the fallback render-model box must not affect physics insertion.
   add(m,origin,angles,[-100000]*3,[100000]*3,6,0,i,'changed-model-box',i)
 for m,name in enumerate(names):
  for handle in [0,1,65534,65535]:add(m,[128,-256,512],[0,0,0],[-4,-8,-16],[4,8,16],2,2,handle,'box-zero-angle')
  if assets[name]['phy']:
   for j in range(8):add(m,[rng.uniform(-10000,10000) for _ in range(3)],[rng.uniform(-720,720) for _ in range(3)],[-100]*3,[100]*3,6,0,[0,1,65534,65535][j%4],'physics-poses')
 exe=OUT/'prop_partition_registration_native_x86.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==0x14c and b'asInvoker' in raw,'Native executable identity')
 expected=bytearray(len(rows)*56);physics=bytearray(len(rows)*24);records=[];dummy=direct['models'][0]['phy']
 for m,queries in enumerate(groups):
  case=run/('%03d'%m);case.mkdir();q=case/'queries.bin';q.write_bytes(b''.join(w for _,w in queries));output=case/'native.bin';args=[str(exe),str(GAME/'bin/engine.dll'),str(libs),assets[names[m]]['phy'] or dummy,str(q),str(output)]
  result=subprocess.run(args,cwd=run,capture_output=True,text=True,timeout=60);(case/'native.log').write_text(result.stdout+result.stderr);require(result.returncode==0,'Native registration failed '+str(case)+' '+result.stdout+result.stderr)
  data=output.read_bytes();reference=Path(str(output)+'.physics').read_bytes();require(len(data)==len(queries)*56 and len(reference)==len(queries)*24,'Result extent')
  for j,(index,_) in enumerate(queries):expected[index*56:(index+1)*56]=data[j*56:(j+1)*56];physics[index*24:(index+1)*24]=reference[j*24:(j+1)*24]
  records.append(dict(model=names[m],queries=len(queries),path=str(case),sha256=sha(output)))
  if (m+1)%10==0:print('PASS original registration/physics models',m+1,flush=True)
 totals=collections.Counter();benchmark=collections.Counter();baseline={};changed=0
 for i,row in enumerate(rows):
  values=struct.unpack_from('<8I6f',expected,i*56);registered=int(row['solid']==2 or row['solid']==6 and row['cache']==0);missing=row['solid']==6 and row['cache']!=0
  policy=(registered,0 if missing else row['solid'],0x65 if registered else 0,row['handle'] if registered else 65535,int(row['solid']==6),int(row['solid']==6 and row['cache']==0),int(missing),int(missing))
  require(values[:8]==policy,'Policy mismatch row '+str(i)+' '+str((values[:8],policy)))
  if not registered:require(expected[i*56+32:(i+1)*56]==bytes(24),'Skipped insertion has recorded bounds')
  if values[5]:require(expected[i*56+32:(i+1)*56]==physics[i*24:(i+1)*24],'Physics forwarding mismatch')
  if row['label']=='box-zero-angle':require(values[8:]==tuple(row['origin'][j]+v for k in ['mins','maxs'] for j,v in enumerate(row[k])),'Zero-angle fallback box')
  if row['label']=='benchmark':
   benchmark['props']+=1;benchmark['insertions']+=registered;benchmark['nonsolid']+=row['solid']==0;benchmark['missing_disabled']+=missing;baseline[row['ordinal']]=expected[i*56:(i+1)*56]
  if row['label']=='changed-model-box':require(baseline[row['ordinal']]==expected[i*56:(i+1)*56],'Model bounds replaced physics bounds');changed+=1
  totals['queries']+=1;totals['insertions']+=registered;totals['physics_aabb_calls']+=values[5];totals['missing_disabled']+=missing;totals['returned_invalid_handle']+=registered and row['handle']==65535;totals[row['label']]+=1
 require(dict(benchmark)==dict(props=485,insertions=145,nonsolid=339,missing_disabled=1),'Benchmark registration counts')
 (run/'native-ordered.bin').write_bytes(expected);(run/'direct-physics-ordered.bin').write_bytes(physics);(run/'cases.json').write_text(json.dumps(rows,indent=2))
 # Repeat the first model group for deterministic bytes; the initial pass
 # above covers all79 models. This is not a repeat of the complete corpus.
 repeat=run/'repeat.bin';case=run/'000';result=subprocess.run([str(exe),str(GAME/'bin/engine.dll'),str(libs),assets[names[0]]['phy'] or dummy,str(case/'queries.bin'),str(repeat)],cwd=run,capture_output=True,text=True,timeout=60);(run/'repeat.log').write_text(result.stdout+result.stderr);require(result.returncode==0 and repeat.read_bytes()==(case/'native.bin').read_bytes() and Path(str(repeat)+'.physics').read_bytes()==(case/'native.bin.physics').read_bytes(),'Repeated native path changed')
 for p,s in originals.items():require(sha(Path(p))==s,'Original changed after test '+p)
 sources={str(ROOT/'work'/p):sha(ROOT/'work'/p) for p in ['probe_static_prop_partition_registration_native.cpp','build_static_prop_partition_registration.cmd','verify_static_prop_partition_registration.py','native_private_world_image_fixture.h','native_vphysics_library_fixture.h']}
 result=dict(status='PASS',path=str(run),benchmark=dict(benchmark),totals=dict(totals),changed_model_box_cases=changed,policy_differences=0,physics_forwarding_differences=0,executable=dict(path=str(exe),sha256=sha(exe),architecture='x86 test-only original oracle'),sources=sources,original_hashes=originals,records=records,prior_scope_receipt=str(PRIOR),prior_scope_receipt_sha256=sha(PRIOR),limitations=['Original whole registration and physics GetAABB execute with unchanged code; model/cache/name/log/partition services are controlled fixtures.','CreateHandle is a recorder, not the original spatial partition. No candidate enumeration, query ordering, full-scene trace or live SFM proof.','GetAABB values are native characterization data, not independently reconstructed owned x64 bounds math.','Missing collision scope is the prior proven explicit loose GAME mounts; dummy valid PHY loaded for absent-model harness is never provided to registration.','Original sets solidType0 for null/empty vcollide and leaves initial partition handleFFFF; owned registry not modified here.','Returned handleFFFF is stored by original without retry; recorded request does not prove successful partition allocation.','No editor build/integration or serialization/animation/rendering/save changes.'])
 (run/'results.json').write_text(json.dumps(result,indent=2));print('PASS '+str(run/'results.json'),flush=True)
if __name__=='__main__':main()
