"""Opt-in actual pilot VMT CPU/runtime composition, not native full-frame parity."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,math,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-pilot-runtime-20261027'
CLIENT=ROOT/'outputs/material-proxy-evaluation-20261027/copied-bin/client.dll'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');SESSION=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_session_file_runtime_candidate.h','test_material_pilot_runtime.cpp','build_material_pilot_runtime.cmd','verify_material_pilot_runtime.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def f32(bits):return struct.unpack('<f',struct.pack('<I',bits))[0]
def main():
 pre=json.loads((OUT/'preflight.json').read_text());parent=json.loads(Path(pre['parent']).read_text());need(sha(pre['parent'])==pre['parent_sha256']=='cc1c125294c314e216e32b447aec2378fd1956eefd8ad472e6a640d821145329','accepted parent')
 protected={k:v for k,v in pre['protected'].items() if k not in pre['intended_changes']};need(all(sha(k)==v for k,v in protected.items()),'all unrelated source/inputs before')
 need(sha(OUT/'before/material_object_store_candidate.h')==parent['source'][str(ROOT/'work/material_object_store_candidate.h')],'preceding store snapshot')
 need(sha(CLIENT)=='91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b' and sha(IFM)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','historical proxy source images')
 need(sha(SESSION)=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','read-only benchmark')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True);sources=source_closure([Path(k) for k in parent['source']]+[ROOT/'work'/n for n in NEW])
 client,cr=check_ranges(CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[('Sine_registration',0x10492170,0x10492189),('Sine_Init',0x10136440,0x101364e4),('Sine_OnBind',0x101364f0,0x101365a7),('Equals_registration',0x10492190,0x104921a9),('Equals_Init',0x100e5fa0,0x100e6033),('Equals_OnBind',0x101373a0,0x1013742d),('TextureTransform_Init',0x10137ad0,0x10137baf),('TextureTransform_OnBind',0x10137c30,0x10137e3d),('TextureScroll_Init',0x10198ab0,0x10198b58),('TextureScroll_OnBind',0x10198b80,0x10198d40),('weapon_registration',0x104a21b0,0x104a21c9),('weapon_Init',0x10265100,0x10265126),('weapon_bind_wrapper',0x10175ee0,0x10175f0f),('weapon_bind_body',0x1028c510,0x1028c5a0)])
 need(client.string(0x105000c4)=='Sine' and client.string(0x105000cc)=='Equals' and client.string(0x10576dc4)=='weapon_invis','actual registered spellings')
 _,ir=check_ranges(IFM,'ifm-disassembly.txt',[('puppet_vtable',0x10243220,0x10243241),('renderable_to_unknown',0x10242660,0x10242664),('unknown_to_base_entity_null',0x1022b000,0x1022b003),('draw_packet_puppet',0x10243a8a,0x10243aa9)])
 (run/'static-reference.json').write_text(json.dumps(dict(client=cr,ifm=ir),indent=2))
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_pilot_runtime.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh seven owned x64 builds with all required owned assembly')
 exes={}
 for tag,name in [('pilot','pilot_runtime_x64'),('owned','bindings_owned_x64'),('batch','batch_regression_x64'),('single','single_regression_x64'),('store','store_regression_x64'),('parameters','parameters_regression_x64'),('cached','cached_regression_x64')]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b and b'asInvoker' in raw,'fresh x64 manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean compile '+tag)
  inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspect);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.M|re.I);need(modules==['KERNEL32.dll'],'system-only imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine='0x8664',imports=modules)
 def execute(tag,args):
  p=subprocess.run([exes[tag]['path']]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=120);(run/(tag+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,tag+' failed: '+p.stderr);return p.stdout.strip()
 pilot=execute('pilot',[SESSION,GAME,run/'pilot.tsv']);m=re.search(r'checks=(\d+) session_elements=(\d+) session_models=(\d+)',pilot);need(m and int(m[1])==132 and int(m[2])==int(m[3])==10,'actual pilot integration coverage')
 regressions={tag:execute(tag,['--test'] if tag in ['store','parameters'] else []) for tag in ['owned','batch','single','store','parameters','cached']}
 for tag,needle in [('owned','checks=606 allocation_failures=572'),('batch','checks=1577 actual_allocation_failures=1519'),('single','checks=888 actual_allocation_failures=788'),('store','checks=4657'),('parameters','checks=281'),('cached','checks=20 plus_prior=49')]:need(needle in regressions[tag],'preceding regression '+tag)
 reads={};materials=[];models=[];frames=[];schema=None
 for line in (run/'pilot.tsv').read_text().splitlines():
  v=line.split('\t')
  if v[0]=='F':
   p=Path(v[1]);data=p.read_bytes();h=14695981039346656037
   for byte in data:h=((h^byte)*1099511628211)&0xffffffffffffffff
   need(len(data)==int(v[2]) and h==int(v[3]),'read-only file bytes unchanged after audit');reads[str(p)]=hashlib.sha256(data).hexdigest()
  elif v[0]=='M':materials.append(list(map(int,v[1:])))
  elif v[0]=='O':models.append(dict(id=int(v[1]),asset=v[2]))
  elif v[0]=='S':schema=v[1:]
  elif v[0]=='T':frames.append(list(map(int,v[1:])))
 need(len(materials)==len(models)==10 and len({v[1] for v in materials})==1 and schema==['UnlitTwoTexture_DX9','24','7'] and len(frames)==7,'complete real pilot resource/schema/frame evidence')
 # Independent approximate numerical checks complement prior exact native
 # component proofs. They do not establish exact native whole-frame composition.
 errors=[]
 for q in frames:
  t,a,b,tx,ty,sx,sy=map(f32,q[:7]);need(q[7]==6 and q[2]==q[3] and q[1]==q[4],'ordered exact Equals component forwarding')
  expected1=(math.sin(t*2*math.pi/f32(0x3dcccccd))*.5+.5)*(.02-(-.02))-.02
  expected2=(math.sin(t*2*math.pi/7)*.5+.5)*(-.005-.005)+.005
  errors.extend([abs(a-expected1),abs(b-expected2)])
  for actual,expected in [(sx,math.cos(87*math.pi/180)*t*3),(sy,math.sin(87*math.pi/180)*t*3)]:errors.append(abs(actual-(expected-math.floor(expected))))
 need(max(errors)<3e-6,'independent bounded analytical proxy/matrix values')
 need(all(sha(k)==v for k,v in sources.items()) and all(sha(k)==v for k,v in protected.items()) and all(sha(k)==v for k,v in reads.items()),'complete new/unrelated/source/original input audit')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/k)==v for k,v in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 result=dict(status='VERIFIED actual pilot VMT CPU initialization and checked owned evaluation; native full composition/GPU PARTIALLY VERIFIED',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',pilot_checks=int(m[1]),session_elements=10,session_models=10,shared_resources=1,parameter_count=24,proxy_instances=7,evaluated_times=7,journal_writes=42,maximum_independent_analytic_error=max(errors),pilot=pilot,regressions=regressions,read_inputs=reads,source=sources,protected=protected,parent=pre['parent'],parent_sha256=pre['parent_sha256'],executables=exes,editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,static_reference_sha256=sha(run/'static-reference.json'),frame_evidence_sha256=sha(run/'pilot.tsv'),limits=['Existing explicit puppet factory implements only Sine/Equals/TextureTransform/TextureScroll plus verified puppet-without-game-entity weapon_invis branch; not a general client proxy registry','Actual loose VMT and ten real benchmark element/model identities used, original declarations retained; source shader alias falls back to UnlitTwoTexture_DX9','Clock is explicitly supplied test material time, not a verified session frame/shot-to-engine-clock mapping','Native component/factory/context proofs reused and original bytes rechecked; new composed real-material test is owned CPU plus approximate independent analytic checks, NOT an exact native full-frame or GPU/visual comparison','Texture resources, all session typed overrides, production renderer admission, other TF proxies, other unsupported shaders and full open/edit/save/reopen workflow remain incomplete','New filename service separates resource find/creation from explicit CPU initialization; cached lookup does not AddRef/init, unavailable files/services are not known zero resources','Store and service must outlive callbacks on owning thread; no global loader/registry rollback or document/history redesign','No original writes/SFM/computer use/editor launch/GPU/dependency installation/publication'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','pilot_checks','session_elements','session_models','proxy_instances','evaluated_times','journal_writes','maximum_independent_analytic_error']},indent=2),flush=True)
if __name__=='__main__':main()
