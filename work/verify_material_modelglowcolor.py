"""Registered ModelGlowColor alias and independent x64 store execution; no SFM UI."""
from pathlib import Path
from datetime import datetime,timezone
import copy,hashlib,json,random,re,struct,subprocess,tempfile,time
import verify_material_yellowlevel as yellow_prior
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_material_object_parameters import node,string,packet,text,decode
from verify_material_equals_evaluation import decode_frames
import verify_material_multiply as prior_util
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-modelglowcolor-proxy-20261027'
CLIENT=prior_util.CLIENT;MATERIAL=prior_util.MATERIAL;LIBS=prior_util.LIBS;DUMP=prior_util.DUMP
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def declaration(result):return node('YellowLevel',[string('resultVar',result)])
def encode_frames(cases):
 out=bytearray(struct.pack('<I',len(cases)))
 for c in cases:
  out+=struct.pack('<2I',c['index'],c['initialized'])
  for frame in c['frames']:
   out+=struct.pack('<I',len(frame['writes']))+b''.join(struct.pack('<7I',*w) for w in frame['writes'])+text(frame['shader'])+struct.pack('<I',len(frame['values']))
   out+=b''.join(struct.pack('<28I',*v['words'])+v['text'].encode('ascii')+v['name'].encode('ascii') for v in frame['values'])+struct.pack('<7I',*frame['forward'])
 return bytes(out)
def corpus():
 base,vmt=yellow_prior.corpus();cases=[]
 for c in base:
  if c['kind']=='chain':continue
  c=copy.deepcopy(c)
  for d in c['root']['children'][-1]['children']:
   if d['key']=='YellowLevel':d['key']='ModelGlowColor'
  cases.append(c)
 # Both registered names on one resource must invoke the SAME complete setter.
 for c in list(cases):
  if c['label'] in ['float','int','string','vec1','vec2','vec3','vec4','vec4-ones','matrix','undefined','color']:
   c=copy.deepcopy(c);c['kind']='alias';c['label']='two-registered-names';c['root']['children'][-1]['children'].append(node('YellowLevel',[string('resultVar',c['output'])]));cases.append(c)
 source=vmt.read_text();names=['ModelGlowColor','Equals','Equals','YellowLevel','Multiply'];blocks=[];equals=re.findall(r'"Equals"\s*\{([^{}]+)\}',source);need(len(equals)==2,'two actual Equals declarations');ei=0
 for name in names:
  if name=='Equals':block=equals[ei];ei+=1
  else:
   found=re.findall('"'+name+r'"\s*\{([^{}]+)\}',source);need(len(found)==1,'actual '+name+' declaration');block=found[0]
  blocks.append((name,dict(re.findall(r'"(\w+)"\s*"([^"]*)"',block))))
 expected=[('ModelGlowColor',{'resultVar':'$glowcolor'}),('Equals',{'srcVar1':'$glowcolor','resultVar':'$selfillumtint'}),('Equals',{'srcVar1':'$glowcolor','resultVar':'$color2'}),('YellowLevel',{'resultVar':'$yellow'}),('Multiply',{'srcVar1':'$color2','srcVar2':'$yellow','resultVar':'$color2'})]
 need(blocks==expected,'complete actual color-proxy subsequence')
 for glow in ['.25','[.25 -.5 .75]','[1 1 1 0]']:
  for color in ['[.25 .5 1]','[1 0 -1]','[.125 .25 .5]']:
   for initial in ['.25','[.25 -.5 .75]','[1 1 1 0]']:
    for bind in [0,1,0xfffffff0,0xdeadbeef]:
     ds=[node(name,[string(key,value) for key,value in fields.items()]) for name,fields in blocks]
     root=node('UnlitTwoTexture',[string('$glowcolor',glow),string('$selfillumtint','[.125 .25 .5]'),string('$color2',color),string('$yellow',initial),node('Proxies',ds)])
     cases.append(dict(label='actual-five-declaration-chain-controlled-inputs',kind='chain',bind=bind,output='$glowcolor',root=root))
 return cases,vmt
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(p)==h for p,h in pre['protected'].items()),'protected sources/inputs before')
 need(sha(pre['parent'])==pre['parent_sha256']=='28d3c78aca773dd91b05921e51d2258dbe770fe3088882a14f23a3455e67b7db','accepted YellowLevel parent');parent=json.loads(Path(pre['parent']).read_text())
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in ['material_modelglowcolor_proxy_candidate.h','probe_material_modelglowcolor.cpp','build_material_modelglowcolor.cmd','verify_material_modelglowcolor.py']])
 need(sha(CLIENT)=='91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b','pinned original client')
 need(all(sha(p)==h for p,h in parent['read_inputs'].items()),'original/private reference inputs')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 image,ranges=check_ranges(CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[('ModelGlowColor_registration',0x1049d9a0,0x1049d9b9),('factory',0x10225b20,0x10225b46),('shared_result_Init',0x100e5db0,0x100e5ea9),('whole_OnBind',0x102247b0,0x102247d9),('constructor',0x100e6220,0x100e6230),('GetMaterial',0x100e5f90,0x100e5f9a),('Release',0x10175ed0,0x10175ede)])
 need(image.string(0x1053572c)=='ModelGlowColor' and image.word(0x104ba584)==0x3f800000 and [image.word(0x10535524+4*i) for i in range(5)]==[image.word(0x1053550c+4*i) for i in range(5)]==[0x100e5db0,0x102247b0,0x10175ed0,0x100e5f90,0x10137780],'registered factory/vtable/constant')
 (run/'static-reference.json').write_text(json.dumps(ranges,indent=2));started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_modelglowcolor.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh native and owned builds')
 exes={}
 for tag,name,machine,magic in [('native','modelglowcolor_reference_x86',0x14c,0x10b),('owned','modelglowcolor_owned_x64',0x8664,0x20b)]:
  exe=OUT/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic and b'asInvoker' in b,'fresh architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error) C\d+',log),'clean compile '+tag)
  inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspect);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.I|re.M);need(imports==['KERNEL32.dll'],'system-only direct imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=imports,fresh=True)
 def execute(exe,args,label):
  p=subprocess.run([str(exe)]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+p.stdout+p.stderr);return p.stdout.strip()
 safety=execute(exes['owned']['path'],[],'owned-safety');m=re.search(r'checks=(\d+) actual_allocation_failures=(\d+)',safety);need(m and int(m[1])>=48 and int(m[2])>0,'actual store safety/allocation sweep')
 weapon=execute(exes['owned']['path'],['--weapon-refusal',Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')],'actual-weapon-refusal');need('glow_instances=2 yellow_instances=2 multiply_instances=2 complete_materials=REFUSED_SAFE' in weapon,'complete original weapon materials still refuse')
 allCases,vmt=corpus();admissionCases=[c for c in allCases if '\0' in c['label']];cases=[c for c in allCases if '\0' not in c['label']];nativeCases=cases+admissionCases;need(len(admissionCases)==44,'embedded-null source-admission gap retained as explicit native-only observations')
 query=run/'queries.bin';nativeQuery=run/'native-queries.bin'
 for p,cs in [(query,cases),(nativeQuery,nativeCases)]:p.write_bytes(struct.pack('<I',len(cs))+b''.join(struct.pack('<I',c['bind'])+packet(c['root']) for c in cs))
 (run/'cases.json').write_text(json.dumps(nativeCases,indent=2))
 cpu=parent['reused_cpu'];need(sha(cpu['path'])==cpu['sha256'],'accepted original CPU baseline identity');cpuQuery=run/'cpu-query.bin';cpuQuery.write_bytes(struct.pack('<I',len(nativeCases))+b''.join(struct.pack('<4I',0,0,0,0)+struct.pack('<12I',0,0,0,0,0,0,0,0,1,0,92,0)+text('modelglow/case')+packet(c['root'])+struct.pack('<6I',0,0,0,0,0,0) for c in nativeCases));mount=run/'mount';mount.mkdir();cpuOut=run/'cpu-original.bin';cpuLog=execute(cpu['path'],[MATERIAL,LIBS,mount,cpuQuery,cpuOut],'original-cpu');n,records=decode(cpuOut.read_bytes());allInitial=[r for r in records if r['stage']==1];need(n==len(nativeCases)==len(allInitial) and all(r['result'] for r in allInitial),'all separately executed original CPU initial states');initial=allInitial[:len(cases)]
 prepared=run/'prepared.bin';nativePrepared=run/'native-prepared.bin'
 for p,states in [(prepared,initial),(nativePrepared,allInitial)]:p.write_bytes(struct.pack('<I',len(states))+b''.join(text(s['shader'])+struct.pack('<I',len(s['values']))+b''.join(struct.pack('<28I',*v['words'])+v['text'].encode('ascii')+v['name'].encode('ascii') for v in s['values']) for s in states))
 fullNative=run/'native-full.bin';native=run/'native-admitted.bin';owned=run/'owned.bin';nativeLog=execute(exes['native']['path'],[CLIENT,nativeQuery,nativePrepared,fullNative],'native-whole');ownedLog=execute(exes['owned']['path'],[run/'absent.dll',query,prepared,owned],'owned-whole')
 allDecoded=decode_frames(fullNative.read_bytes());need(encode_frames(allDecoded)==fullNative.read_bytes(),'complete lossless reference wire re-encoding before selecting shared admission scope');native.write_bytes(encode_frames(allDecoded[:len(cases)]))
 if native.read_bytes()!=owned.read_bytes():
  a=decode_frames(native.read_bytes());b=decode_frames(owned.read_bytes());bad=next(i for i in range(len(a)) if a[i]!=b[i]);(run/'first-mismatch.json').write_text(json.dumps(dict(index=bad,case=cases[bad],native=a[bad],owned=b[bad]),indent=2));raise RuntimeError('ModelGlow whole trace mismatch '+str(bad)+' '+cases[bad]['label'])
 for c in allDecoded[len(cases):]:need(c['initialized'] and len(c['frames'])==3 and all(f['writes'][0][1:]==[3,0,0x3f800000,0x3f800000,0x3f800000,0] for f in c['frames']),'native embedded-null prefix accepted and writes full constant vector; owned source-admission refusal is not declared equivalent')
 decoded=decode_frames(native.read_bytes());frames=0;refused=0;independent=0;chainFrames=0;aliasFrames=0;preservedCount=0
 for c,state,actual in zip(cases,initial,decoded):
  if c['kind']=='missing':need(not actual['initialized'],'actual missing result refusal');refused+=1;continue
  need(actual['initialized'],'all bounded string result declarations initialize');old=next(v for v in state['values'] if v['name']==c['output']);oldWords=old['words'];unchanged=(oldWords[1]&15)==2 and oldWords[3:7]==[0x3f800000]*3+[0]
  for frame in actual['frames']:
   event=frame['writes'][0];need(event[1:]==[3,0,0x3f800000,0x3f800000,0x3f800000,0],'unconditional full SetVec3 call independent of output suffix and bind argument')
   v=next(v for v in frame['values'] if v['name']==c['output']);q=v['words'];need(q[3:7]==[0x3f800000]*3+[0] and q[2]==1 and not q[8] and not v['text'],'whole checked result converts/clears prior numeric/string/matrix state')
   expected=oldWords[1] if unchanged else ((oldWords[1]&~0x7f)|0x32);need(q[1]==expected,'native value fast path preserves old count only when all four cached values already equal');preservedCount+=unchanged and (expected&0x70)!=0x30
   if c['kind']=='chain':
    oldColor=next(v for v in state['values'] if v['name']=='$color2');color=next(v for v in frame['values'] if v['name']=='$color2');need(len(frame['writes'])==5 and [w[1] for w in frame['writes']]==[3,5,5,3,5] and color['words'][3:7]==[0x3f800000]*3+[0] and next(v for v in frame['values'] if v['name']=='$selfillumtint')['words'][3:7]==[0x3f800000]*3+[0],'actual five-proxy color subsequence produces ones in tint/color and consumes real YellowLevel output');chainFrames+=1
   elif c['kind']=='alias':need(len(frame['writes'])==2 and all(w[1:]==[3,0,0x3f800000,0x3f800000,0x3f800000,0] for w in frame['writes']),'both registered names share full setter semantics');aliasFrames+=1
   else:need(len(frame['writes'])==1,'single declaration makes one actual setter call')
   independent+=3;frames+=1
 need(refused==12 and preservedCount>0 and chainFrames==324 and aliasFrames==132,'refusal and native fast-path/actual declaration-chain coverage')
 old=parent['executables']['owned'];need(sha(old['path'])==old['sha256'],'accepted YellowLevel owned executable');regression=execute(old['path'],[],'yellowlevel-reused');need('checks=49 actual_allocation_failures=14' in regression,'unchanged YellowLevel safety')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()) and all(sha(p)==h for p,h in parent['read_inputs'].items()),'all sources/protected/reference inputs unchanged after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 limits=['Independent reconstruction remains INCOMPLETE. Only exact registered ModelGlowColor alias and controlled color subsequence admitted to a new opt-in factory; no editor/GPU/complete weapon claim',
 'ModelGlowColor and YellowLevel vtables have identical five method pointers. Whole original ModelGlowColor factory/Init/Bind/Release execute unchanged RX with explicit host KV/material/value services. No DllMain, original imports or live SFM; clock pointer NULL and invalid/non-entity bind arguments used',
 'Both names reuse one existing owned implementation. All compared cases execute actual owned store/proxy frames from separately executed original canonical CPU states. Typed setter services inherit earlier value proofs; shared forwarding projections are not original renderer execution',
 'Bounded admitted string resultVar names<=255; suffix component ignored by both aliases. Alpha renderer Init side effects, nonstring KV configuration, long names and texture/resource ownership remain outside scope',
 'Existing source policy rejects embedded null strings before proxy Init.44 native-only cases accept the prefix and write constants. They are retained as an explicit admission difference, not equivalent cases; full native wire is losslessly re-encoded before selecting common-admission prefix',
 'Whole color chain uses five actual VMT declarations with controlled supplied glow/yellow/tint/color inputs; other full VMT effects are not removed in actual complete-material refusal tests. Native SetVec3 equality fast path can preserve vector4 count metadata',
 'Actual red/blue complete VMTs construct ModelGlowColor,YellowLevel,Multiply but safely refuse AnimatedWeaponSheen and invis. Constructor success does not prove all supported initializers/parameter lookups or full weapon rendering work',
 'Old factories and earlier three/four/five-name refusal evidence remain unchanged. Previous YellowLevel executable reused by accepted hash, not rebuilt. Editor313-source closure unchanged; no original writes, SFM/computer use, new dependency, history/serialization/renderer changes or publication']
 result=dict(status='VERIFIED registered ModelGlowColor alias and controlled color chain; full material/renderer INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),frames=frames,init_refused=refused,independent_checks=independent,actual_chain_frames=chainFrames,registered_alias_frames=aliasFrames,preserved_vector4_count_frames=preservedCount,exact_bytes=native.stat().st_size,trace_sha256=sha(native),native_only_admission_gap_cases=len(admissionCases),native_only_admission_gap_frames=len(admissionCases)*3,complete_native_trace_sha256=sha(fullNative),safety=safety,safety_checks=int(m[1]),allocation_failures=int(m[2]),native_log=nativeLog,owned_log=ownedLog,cpu_log=cpuLog,actual_weapon_refusal=weapon,executables=exes,reused_cpu=cpu,reused_yellowlevel=old,yellowlevel_regression=regression,source=sources,protected=pre['protected'],read_inputs={str(vmt):sha(vmt),**parent['read_inputs']},parent=pre['parent'],parent_sha256=pre['parent_sha256'],editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,static_sha256=sha(run/'static-reference.json'),query_sha256=sha(query),prepared_sha256=sha(prepared),limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','frames','init_refused','independent_checks','actual_chain_frames','registered_alias_frames','preserved_vector4_count_frames','safety_checks','allocation_failures','exact_bytes','yellowlevel_regression']},indent=2),flush=True)
if __name__=='__main__':main()
