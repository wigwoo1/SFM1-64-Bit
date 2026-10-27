"""Whole original Multiply vs owned x64 factory and bounded raw-value plan."""
from pathlib import Path
from datetime import datetime,timezone
import copy, hashlib,json,random,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_material_object_parameters import node,string,packet,text,decode
from verify_material_equals_evaluation import decode_frames
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-multiply-proxy-20261027'
CLIENT=ROOT/'outputs/material-proxy-evaluation-20261027/copied-bin/client.dll'
MATERIAL=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
LIBS=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def declaration(out='$out',a='$src',b='$other'):
 return node('Multiply',[string('resultVar',out),string('srcVar1',a)]+([string('srcVar2',b)] if b is not None else []))
def value(name,kind):
 if kind=='float':return node(name,kind=3,bits=bits(.375))
 if kind=='int':return node(name,kind=2,bits=0xfffffffd)
 if kind=='string':return string(name,'12.75 trailing')
 if kind.startswith('vec'):return string(name,'['+' '.join(['.125','.25','.5','1'][:int(kind[-1])])+']')
 raise RuntimeError(kind)
def corpus():
 cases=[]
 def add(label,outKind='float',aKind='float',bKind='float',out='$out',a='$src',b='$other',decls=None):
  children=[]
  for name,kind in [('$out',outKind),('$src',aKind),('$other',bKind)]:
   if kind!='undefined':children.append(value(name,kind))
  if outKind=='undefined' and out=='$out':out='$texture2'
  if aKind=='undefined' and a=='$src':a='$basetexture'
  if bKind=='undefined' and b=='$other':b='$texture2'
  children.append(node('Proxies',decls or [declaration(out,a,b)]));cases.append(dict(mode=0,label=label,root=node('UnlitTwoTexture',children)))
 for o in ['float','int','string','vec1','vec2','vec3','vec4','undefined']:
  for a in ['float','int','string','vec4','undefined']:
   for b in ['float','int','string','vec4','undefined']:add('typed',o,a,b)
 for n in range(1,5):
  for c in ['[-7]','[-1]','[0]','[1]','[2]','[3]','[foo]','[ 1junk','[+01]']:
   add('component','vec'+str(n),out='$out'+c)
 for o,a,b in [('$out','$out','$other'),('$out','$src','$out'),('$out','$out','$out')]:
  for k in ['float','int','vec1','vec2','vec3','vec4']:add('alias',k,k,k,o,a,b)
 for out,a,b in [('$missing','$src','$other'),('$out','$missing','$other'),('$out','$src','$missing'),('$out','$src',None),('$out','$src',''),('$out','$src[0]','$other')]:add('init-refusal',out=out,a=a,b=b)
 add('chain','vec3','vec4','float',decls=[declaration('$out','$src','$other'),declaration('$out','$out','$src')])
 # Raw cache/type cases reuse one separately prepared original canonical state.
 # They exercise the shared value plan, NOT arbitrary-state store admission.
 raw=[];special=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x3f800000,0xbf800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7fa12345,0xffa54321]
 for t in [0,4,0x12,0x22,0x32,0x42,1,3,5,6,7]:
  for a in special:
   for b in special:raw.append(dict(type=t,a=[a,0,0,0],b=[b,0,0,0],ai=a,bi=b,firstType=0,secondType=0))
 rng=random.Random(2026102781)
 for i in range(2048):
  t=rng.choice([0,4,0x12,0x22,0x32,0x42,6]);raw.append(dict(type=t,a=[rng.getrandbits(32) for _ in range(4)],b=[rng.getrandbits(32) for _ in range(4)],ai=rng.getrandbits(32),bi=rng.getrandbits(32),firstType=rng.choice([0,4,6,0x12,0x22,0x32,0x42]),secondType=rng.choice([0,4,6,0x12,0x22,0x32,0x42])))
 add('raw-template');template=copy.deepcopy(cases.pop())
 for q in raw:cases.append(dict(mode=1,label='raw-cache',root=template['root'],raw=q))
 # Controlled actual weapon Multiply declaration, without claiming other proxies.
 vmt=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\materials\models\weapons\c_items\c_flamethrower_blue.vmt')
 found=re.findall(r'"Multiply"\s*\{([^{}]+)\}',vmt.read_text());need(len(found)==1,'actual weapon Multiply block');fields=dict(re.findall(r'"(\w+)"\s*"([^"]*)"',found[0]));need(fields=={'srcVar1':'$color2','srcVar2':'$yellow','resultVar':'$color2'},'actual weapon declaration')
 for color,yellow in [('[.25 .5 1]','.75'),('[1 0 -1]','0'),('[.125 .25 .5]','1')]:
  root=node('UnlitTwoTexture',[string('$color2',color),string('$yellow',yellow),node('Proxies',[declaration('$color2','$color2','$yellow')])]);cases.append(dict(mode=0,label='actual-declaration-controlled-inputs',root=root))
 return cases,vmt
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(p)==h for p,h in pre['protected'].items()),'all protected source/inputs before')
 parent=json.loads(Path(pre['parent']).read_text());need(sha(pre['parent'])==pre['parent_sha256']=='9ad3b8859f0273ed2804865c5117ae0410217599a403ffbeaaf3965275e474fb','previous bridge receipt')
 need(sha(CLIENT)=='91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b','private original client')
 priorPath=ROOT/'outputs/material-proxy-evaluation-20261027/verification-_rdevn4r/results.json';need(sha(priorPath)=='8efa74b732aa34aa8bcfe0169eb76ab97c644b9a8a5d39698322f465b4e8c56b','accepted earlier client/value proof');prior=json.loads(priorPath.read_text())
 baseline=next(e for e in prior['executables'] if Path(e['path']).name=='unlit_two_texture_parameters_reference_x86.exe');need(sha(baseline['path'])==baseline['sha256'],'exact original CPU baseline executable');need(all(sha(p)==h for p,h in prior['private_library_hashes'].items()),'original library subset pinned')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in ['material_multiply_proxy_candidate.h','material_multiply_scalar_x87_candidate.inc','probe_material_multiply.cpp','build_material_multiply.cmd','verify_material_multiply.py']])
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 image,ranges=check_ranges(CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[('Multiply_registration',0x10492110,0x10492129),('factory',0x10137010,0x10137036),('Init_requires_second',0x10136c20,0x10136c4d),('OnBind_whole',0x10136f00,0x10137001),('shared_two_input_Init',0x100e5fa0,0x100e6033),('effective_type',0x100e6040,0x100e60cc),('basic_result_Init',0x100e5db0,0x100e5ea9),('scalar_result_write',0x100e5eb0,0x100e5f52),('GetMaterial',0x100e5f90,0x100e5f9a),('Release',0x10175ed0,0x10175ede)])
 need(image.string(0x105000a8)=='Multiply' and [image.word(0x104ffeac+4*i) for i in range(5)]==[0x10136c20,0x10136f00,0x10175ed0,0x100e5f90,0x10137780],'actual registered factory/vtable');(run/'static-reference.json').write_text(json.dumps(ranges,indent=2))
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_multiply.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh native/owned builds')
 exes={}
 for tag,name,machine,magic in [('native','multiply_reference_x86',0x14c,0x10b),('owned','multiply_owned_x64',0x8664,0x20b)]:
  path=OUT/(name+'.exe');b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(path.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic and b'asInvoker' in b,'fresh executable '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error) C\d+',log),'clean compile '+tag)
  inspect=subprocess.run([str(DUMP),'/headers','/imports',str(path)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspect);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.I|re.M);need(imports==['KERNEL32.dll'],'system-only imports '+tag);exes[tag]=dict(path=str(path),sha256=sha(path),machine=hex(machine),imports=imports,fresh=True)
 def execute(exe,args,label):
  p=subprocess.run([str(exe)]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=240);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+p.stdout+p.stderr);return p.stdout.strip()
 safety=execute(exes['owned']['path'],[],'owned-safety');m=re.search(r'checks=(\d+) actual_allocation_failures=(\d+)',safety);need(m and int(m[2])>0,'actual store allocation failure sweep')
 weapon=execute(exes['owned']['path'],['--weapon-refusal',Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')],'actual-weapon-refusal');need('actual_weapon_materials=2 multiply_instances=2 full_weapons=REFUSED_SAFE' in weapon,'actual untouched complete weapon materials refuse remaining unsupported effects')
 cases,vmt=corpus();canonical=[c for c in cases if c['mode']==0];rawTemplate=dict(root=node('UnlitTwoTexture',[value('$out','float'),value('$src','float'),value('$other','float'),node('Proxies',[declaration()])]))
 initialRoots=canonical+[rawTemplate];cpuQuery=run/'cpu-query.bin';cpuQuery.write_bytes(struct.pack('<I',len(initialRoots))+b''.join(struct.pack('<4I',0,0,0,0)+struct.pack('<12I',0,0,0,0,0,0,0,0,1,0,92,0)+text('multiply/case')+packet(c['root'])+struct.pack('<6I',0,0,0,0,0,0) for c in initialRoots));mount=run/'mount';mount.mkdir()
 cpuOut=run/'cpu-original.bin';cpuLog=execute(baseline['path'],[MATERIAL,LIBS,mount,cpuQuery,cpuOut],'original-cpu');n,records=decode(cpuOut.read_bytes());initial=[r for r in records if r['stage']==1];need(n==len(initialRoots)==len(initial) and all(r['result'] for r in initial),'original CPU initial states')
 query=run/'query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<I',c['mode'])+packet(c['root']) for c in cases));states=[];ci=0
 for c in cases:
  if not c['mode']:states.append(initial[ci]);ci+=1;continue
  state=copy.deepcopy(initial[-1]);q=c['raw']
  for name,t,integer,vec in [('$out',q['type'],17,[bits(.25)]*4),('$src',q['firstType'],q['ai'],q['a']),('$other',q['secondType'],q['bi'],q['b'])]:
   v=next(v for v in state['values'] if v['name']==name);v['words'][1]=t;v['words'][2]=integer;v['words'][3:7]=vec
  states.append(state)
 prepared=run/'prepared.bin';prepared.write_bytes(struct.pack('<I',len(states))+b''.join(text(s['shader'])+struct.pack('<I',len(s['values']))+b''.join(struct.pack('<28I',*v['words'])+v['text'].encode('ascii')+v['name'].encode('ascii') for v in s['values']) for s in states));(run/'cases.json').write_text(json.dumps(cases,indent=2))
 native=run/'native.bin';owned=run/'owned.bin';nativeLog=execute(exes['native']['path'],[CLIENT,query,prepared,native],'native-whole');ownedLog=execute(exes['owned']['path'],[run/'absent.dll',query,prepared,owned],'owned-whole')
 if native.read_bytes()!=owned.read_bytes():
  a=decode_frames(native.read_bytes());b=decode_frames(owned.read_bytes());bad=next(i for i in range(len(a)) if a[i]!=b[i]);(run/'first-mismatch.json').write_text(json.dumps(dict(index=bad,case=cases[bad],native=a[bad],owned=b[bad]),indent=2));raise RuntimeError('whole trace mismatch case '+str(bad)+' '+cases[bad]['label'])
 decoded=decode_frames(native.read_bytes());frames=sum(len(c['frames']) for c in decoded);refused=sum(not c['initialized'] for c in decoded);need(refused==6,'all missing source/result/second/component Init refusals');checks=0
 for c,s,actual in zip(cases,states,decoded):
  if not actual['initialized']:continue
  if c['mode']==1 and c['raw']['type']==4:
   expected=(c['raw']['ai']*c['raw']['bi'])&0xffffffff;expected=bits(float(expected if expected<0x80000000 else expected-0x100000000));need(actual['frames'][0]['writes'][0][1]==1 and actual['frames'][0]['writes'][0][3]==expected,'independent signed-wrap integer to float setter');checks+=1
  if c['mode']==1 and c['raw']['type']==0x42:
   need(actual['frames'][0]['writes'][0][1:3]==[5,4] and actual['frames'][0]['writes'][0][6]==c['raw']['a'][0],'native vector4 fourth outgoing word is first input component0');checks+=1
 need(checks>500,'independent integer/vector4 assertions')
 bridge=parent['executables']['bridge'];need(sha(bridge['path'])==bridge['sha256'],'accepted bridge executable');regression=execute(bridge['path'],[],'bridge-reused');need('checks=3691 actual_allocation_failures=1822' in regression,'bridge unchanged regression')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()) and all(sha(p)==h for p,h in prior['private_library_hashes'].items()),'all source/protected/private inputs unchanged')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 limits=['Independent reconstruction remains incomplete. New separate opt-in Multiply factory only; no editor/renderer admission, whole shader support or automatic clock/entity contexts',
 'Whole original factory/Init/OnBind/Release execute unchanged RX in isolated x86 reference with explicit host KV/material/value services. DllMain/imports/full SFM process not loaded. Setter/getter cache services inherit prior separately tested value boundary; not complete original MaterialSystem/GPU execution',
 'Factory cases use separately executed original canonical CPU states and actual owned store/proxy frames. Raw cases deliberately patch prepared host variable caches and compare the shared owned result plan and typed setters, not arbitrary nonfinite raw-state admission through the owned store',
 'Float uses original x87 first-return f32 store and second-return multiply rounding/NaN selection; vector uses MULSS second-source destination precedence and observed fourth-word stack alias; integer wraps32 then signed integer-to-float and SetFloat. Default nearest FP modes only; exception flags/alternate precision modes unverified',
 'Alpha result requiring renderer flag/snapshot side effects, component>3, count0/>4 unsafe scratch and resource ownership remain refused. Other effective types perform observed no-write behavior; no omitted named proxy substituted with no-op',
 'Actual blue weapon Multiply declaration tested only with controlled color/yellow values. AnimatedWeaponSheen,invis,ModelGlowColor,YellowLevel remain unsupported. Original five-proxy refusal remains accurate for unchanged older factory; whole weapon and benchmark are still not supported',
 'Parent bridge executable reused by exact accepted hash and rerun, not rebuilt. Full editor and313 dependencies unchanged; no original writes, SFM/computer-use, new dependency, history/serialization/rendering change or publication']
 result=dict(status='VERIFIED bounded Multiply CPU behavior; full material/renderer compatibility INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',safety=safety,safety_checks=int(m[1]),allocation_failures=int(m[2]),cases=len(cases),factory_cases=len(canonical),raw_value_cases=len(cases)-len(canonical),frames=frames,init_refused=refused,independent_checks=checks,exact_bytes=native.stat().st_size,trace_sha256=sha(native),native_log=nativeLog,owned_log=ownedLog,cpu_log=cpuLog,actual_weapon_refusal=weapon,executables=exes,reused_cpu_baseline=baseline,reused_bridge=bridge,bridge_regression=regression,source=sources,protected=pre['protected'],read_inputs={str(vmt):sha(vmt),str(priorPath):sha(priorPath),**prior['private_library_hashes']},parent=pre['parent'],parent_sha256=pre['parent_sha256'],editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,static_sha256=sha(run/'static-reference.json'),query_sha256=sha(query),prepared_sha256=sha(prepared),limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','safety_checks','allocation_failures','cases','factory_cases','raw_value_cases','frames','init_refused','independent_checks','exact_bytes','bridge_regression']},indent=2),flush=True)
if __name__=='__main__':main()
