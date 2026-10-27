"""Whole original YellowLevel and independent x64 store execution; no SFM UI."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,random,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_material_object_parameters import node,string,packet,text,decode
from verify_material_equals_evaluation import decode_frames
import verify_material_multiply as prior_util
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-yellowlevel-proxy-20261027'
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
 cases=[];args=[0,1,0xfffffff0,0xdeadbeef]
 suffixes=['','[-7]','[-1]','[0]','[1]','[2]','[3]','[4]','[999]','[999999999999999999999]','[foo]','[ 1junk','[+01]','[','\0ignored']
 kinds=['float','int','string','vec1','vec2','vec3','vec4','vec4-ones','matrix','undefined','color']
 def field(kind):
  if kind in ['undefined','color']:return []
  if kind=='float':return [node('$out',kind=3,bits=0x3e800000)]
  if kind=='int':return [node('$out',kind=2,bits=0xfffffffd)]
  if kind=='string':return [string('$out','old-value')]
  if kind=='matrix':return [string('$out','[1 0 0 .25 0 1 0 -.5 0 0 1 0 0 0 0 1]')]
  if kind=='vec4-ones':return [string('$out','[1 1 1 0]')]
  return [string('$out','['+' '.join(['.125','.25','.5','1'][:int(kind[-1])])+']')]
 for kind in kinds:
  output='$texture2' if kind=='undefined' else '$color' if kind=='color' else '$out'
  for suffix in suffixes:
   for bind in args:cases.append(dict(label=kind+suffix,kind=kind,bind=bind,output=output,root=node('UnlitTwoTexture',field(kind)+[node('Proxies',[declaration(output+suffix)])])))
 for bind in args:
  for result in ['$missing','',None]:
   d=declaration(result) if result is not None else node('YellowLevel',[])
   cases.append(dict(label='missing-result',kind='missing',bind=bind,output='$out',root=node('UnlitTwoTexture',field('float')+[node('Proxies',[d])])))
 rng=random.Random(2026102791)
 for _ in range(256):
  bits=rng.getrandbits(32)
  if bits&0x7f800000==0x7f800000:bits^=0x00800000
  cases.append(dict(label='varied-finite-initial',kind='float',bind=rng.choice(args),output='$out',root=node('UnlitTwoTexture',[node('$out',kind=3,bits=bits),node('Proxies',[declaration('$out')])])) )
 vmt=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\materials\models\weapons\c_items\c_flamethrower_blue.vmt');source=vmt.read_text();blocks={}
 for name in ['YellowLevel','Multiply']:
  found=re.findall('"'+name+r'"\s*\{([^{}]+)\}',source);need(len(found)==1,'actual '+name+' declaration');blocks[name]=dict(re.findall(r'"(\w+)"\s*"([^"]*)"',found[0]))
 need(blocks['YellowLevel']=={'resultVar':'$yellow'} and blocks['Multiply']=={'srcVar1':'$color2','srcVar2':'$yellow','resultVar':'$color2'},'actual sequence fields')
 for color in ['[.25 .5 1]','[1 0 -1]','[.125 .25 .5]']:
  for initial in ['.25','[.25 -.5 .75]','[1 1 1 0]']:
   for bind in args:
    ds=[node(name,[string(key,value) for key,value in blocks[name].items()]) for name in ['YellowLevel','Multiply']]
    root=node('UnlitTwoTexture',[string('$color2',color),string('$yellow',initial),node('Proxies',ds)]);cases.append(dict(label='actual-declaration-chain-controlled-inputs',kind='chain',bind=bind,output='$yellow',root=root))
 return cases,vmt
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(p)==h for p,h in pre['protected'].items()),'protected sources/inputs before')
 need(sha(pre['parent'])==pre['parent_sha256']=='6d61039eef49c5f86e4504bbd471b7acd5485d64722f2cfe25686f5d8f4ff043','accepted Multiply parent');parent=json.loads(Path(pre['parent']).read_text())
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in ['material_yellowlevel_proxy_candidate.h','probe_material_yellowlevel.cpp','build_material_yellowlevel.cmd','verify_material_yellowlevel.py']])
 need(sha(CLIENT)=='91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b','pinned original client')
 need(all(sha(p)==h for p,h in parent['read_inputs'].items()),'original/private reference inputs')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 image,ranges=check_ranges(CLIENT,'../outputs/material-proxy-evaluation-20261027/client-disassembly-private.txt',[('YellowLevel_registration',0x1049d980,0x1049d999),('factory',0x10225af0,0x10225b16),('shared_result_Init',0x100e5db0,0x100e5ea9),('whole_OnBind',0x102247b0,0x102247d9),('constructor',0x100e6220,0x100e6230),('GetMaterial',0x100e5f90,0x100e5f9a),('Release',0x10175ed0,0x10175ede)])
 need(image.string(0x10535720)=='YellowLevel' and image.word(0x104ba584)==0x3f800000 and [image.word(0x1053550c+4*i) for i in range(5)]==[0x100e5db0,0x102247b0,0x10175ed0,0x100e5f90,0x10137780],'registered factory/vtable/constant')
 (run/'static-reference.json').write_text(json.dumps(ranges,indent=2));started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_yellowlevel.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh native and owned builds')
 exes={}
 for tag,name,machine,magic in [('native','yellowlevel_reference_x86',0x14c,0x10b),('owned','yellowlevel_owned_x64',0x8664,0x20b)]:
  exe=OUT/(name+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==magic and b'asInvoker' in b,'fresh architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error) C\d+',log),'clean compile '+tag)
  inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspect);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.I|re.M);need(imports==['KERNEL32.dll'],'system-only direct imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=imports,fresh=True)
 def execute(exe,args,label):
  p=subprocess.run([str(exe)]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+p.stdout+p.stderr);return p.stdout.strip()
 safety=execute(exes['owned']['path'],[],'owned-safety');m=re.search(r'checks=(\d+) actual_allocation_failures=(\d+)',safety);need(m and int(m[1])>=48 and int(m[2])>0,'actual store safety/allocation sweep')
 weapon=execute(exes['owned']['path'],['--weapon-refusal',Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')],'actual-weapon-refusal');need('yellow_instances=2 multiply_instances=2 complete_materials=REFUSED_SAFE' in weapon,'complete original weapon materials still refuse')
 allCases,vmt=corpus();admissionCases=[c for c in allCases if '\0' in c['label']];cases=[c for c in allCases if '\0' not in c['label']];nativeCases=cases+admissionCases;need(len(admissionCases)==44,'embedded-null source-admission gap retained as explicit native-only observations')
 query=run/'queries.bin';nativeQuery=run/'native-queries.bin'
 for p,cs in [(query,cases),(nativeQuery,nativeCases)]:p.write_bytes(struct.pack('<I',len(cs))+b''.join(struct.pack('<I',c['bind'])+packet(c['root']) for c in cs))
 (run/'cases.json').write_text(json.dumps(nativeCases,indent=2))
 cpu=parent['reused_cpu_baseline'];need(sha(cpu['path'])==cpu['sha256'],'accepted original CPU baseline identity');cpuQuery=run/'cpu-query.bin';cpuQuery.write_bytes(struct.pack('<I',len(nativeCases))+b''.join(struct.pack('<4I',0,0,0,0)+struct.pack('<12I',0,0,0,0,0,0,0,0,1,0,92,0)+text('yellow/case')+packet(c['root'])+struct.pack('<6I',0,0,0,0,0,0) for c in nativeCases));mount=run/'mount';mount.mkdir();cpuOut=run/'cpu-original.bin';cpuLog=execute(cpu['path'],[MATERIAL,LIBS,mount,cpuQuery,cpuOut],'original-cpu');n,records=decode(cpuOut.read_bytes());allInitial=[r for r in records if r['stage']==1];need(n==len(nativeCases)==len(allInitial) and all(r['result'] for r in allInitial),'all separately executed original CPU initial states');initial=allInitial[:len(cases)]
 prepared=run/'prepared.bin';nativePrepared=run/'native-prepared.bin'
 for p,states in [(prepared,initial),(nativePrepared,allInitial)]:p.write_bytes(struct.pack('<I',len(states))+b''.join(text(s['shader'])+struct.pack('<I',len(s['values']))+b''.join(struct.pack('<28I',*v['words'])+v['text'].encode('ascii')+v['name'].encode('ascii') for v in s['values']) for s in states))
 fullNative=run/'native-full.bin';native=run/'native-admitted.bin';owned=run/'owned.bin';nativeLog=execute(exes['native']['path'],[CLIENT,nativeQuery,nativePrepared,fullNative],'native-whole');ownedLog=execute(exes['owned']['path'],[run/'absent.dll',query,prepared,owned],'owned-whole')
 allDecoded=decode_frames(fullNative.read_bytes());need(encode_frames(allDecoded)==fullNative.read_bytes(),'complete lossless reference wire re-encoding before selecting shared admission scope');native.write_bytes(encode_frames(allDecoded[:len(cases)]))
 if native.read_bytes()!=owned.read_bytes():
  a=decode_frames(native.read_bytes());b=decode_frames(owned.read_bytes());bad=next(i for i in range(len(a)) if a[i]!=b[i]);(run/'first-mismatch.json').write_text(json.dumps(dict(index=bad,case=cases[bad],native=a[bad],owned=b[bad]),indent=2));raise RuntimeError('Yellow whole trace mismatch '+str(bad)+' '+cases[bad]['label'])
 for c in allDecoded[len(cases):]:need(c['initialized'] and len(c['frames'])==3 and all(f['writes'][0][1:]==[3,0,0x3f800000,0x3f800000,0x3f800000,0] for f in c['frames']),'native embedded-null prefix accepted and writes full constant vector; owned source-admission refusal is not declared equivalent')
 decoded=decode_frames(native.read_bytes());frames=0;refused=0;independent=0;chainFrames=0;preservedCount=0
 for c,state,actual in zip(cases,initial,decoded):
  if c['kind']=='missing':need(not actual['initialized'],'actual missing result refusal');refused+=1;continue
  need(actual['initialized'],'all bounded string result declarations initialize');old=next(v for v in state['values'] if v['name']==c['output']);oldWords=old['words'];unchanged=(oldWords[1]&15)==2 and oldWords[3:7]==[0x3f800000]*3+[0]
  for frame in actual['frames']:
   event=frame['writes'][0];need(event[1:]==[3,0,0x3f800000,0x3f800000,0x3f800000,0],'unconditional full SetVec3 call independent of output suffix and bind argument')
   v=next(v for v in frame['values'] if v['name']==c['output']);q=v['words'];need(q[3:7]==[0x3f800000]*3+[0] and q[2]==1 and not q[8] and not v['text'],'whole checked result converts/clears prior numeric/string/matrix state')
   expected=oldWords[1] if unchanged else ((oldWords[1]&~0x7f)|0x32);need(q[1]==expected,'native value fast path preserves old count only when all four cached values already equal');preservedCount+=unchanged and (expected&0x70)!=0x30
   if c['kind']=='chain':
    oldColor=next(v for v in state['values'] if v['name']=='$color2');color=next(v for v in frame['values'] if v['name']=='$color2');need(len(frame['writes'])==2 and frame['writes'][1][1:3]==[5,3] and color['words'][3:7]==oldColor['words'][3:7],'actual YellowLevel/Multiply declaration order preserves controlled color with yellow ones');chainFrames+=1
   else:need(len(frame['writes'])==1,'single declaration makes one actual setter call')
   independent+=3;frames+=1
 need(refused==12 and preservedCount>0 and chainFrames==108,'refusal and native fast-path/actual declaration-chain coverage')
 old=parent['executables']['owned'];need(sha(old['path'])==old['sha256'],'accepted Multiply owned executable');regression=execute(old['path'],[],'multiply-reused');need('checks=48 actual_allocation_failures=14' in regression,'unchanged Multiply safety')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()) and all(sha(p)==h for p,h in parent['read_inputs'].items()),'all sources/protected/reference inputs unchanged after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 limits=['Opt-in independent CPU YellowLevel implementation only; full SFM workflow/rendering, whole weapon shader/resource and GPU equivalence remain INCOMPLETE',
 'Whole unchanged original registered factory, shared result Init, OnBind, GetMaterial and Release run in private x86 reference with explicit host allocator/KV/material/variable services. No DllMain/imports/live SFM. Original constant1 and unconditional setter path verified by complete instruction bytes',
 'All comparison cases execute actual owned store/proxy frames, starting from separately executed original canonical CPU states; no arbitrary raw-state admission claim. Numeric/string/matrix value service inherits earlier independently checked setters. Forwarding projection uses shared owned value code, not original renderer execution',
 'Original bind argument is NULL or deliberately invalid/non-entity address; global clock pointer is NULL. No entity interface is supplied. Neither YellowLevel nor the paired Multiply reads them. Context-free YellowLevel is measured behavior, not an inferred puppet no-op',
 'Bounded admitted string resultVar names <=255, optional component suffix ignored after base name. Alpha-result renderer flag/snapshot effects, nonstring KV result configuration, long names and texture/resource ownership remain unsupported/refused. Existing whole-material source policy rejects embedded null strings before proxy Init:44 native-only cases accept the prefix and write constants, while owned safety verifies source rejection. This is an explicit compatibility difference, not a passing equivalence claim',
 'Original SetVec3 fast path may keep vector4 count metadata if all four old cache words already match(1,1,1,0). New code uses common typed setter and preserves this behavior; does not force a count repair',
 'Actual weapon VMTs construct YellowLevel and Multiply but refuse three unsupported factory declarations. This does not establish that all supported proxy initializers or parameter lookups succeed in the complete weapon, nor that those three are the only remaining compatibility issues',
 'Separate new factory leaves old factories/expected four- and five-name refusal receipts valid. Multiply safety executable reused by accepted hash, not freshly rebuilt. Editor313-source closure unchanged; no original writes, SFM/computer use, new dependency, history/serialization/rendering changes or publication']
 result=dict(status='VERIFIED bounded YellowLevel CPU contract; full material/renderer compatibility INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),frames=frames,init_refused=refused,independent_checks=independent,actual_chain_frames=chainFrames,preserved_vector4_count_frames=preservedCount,exact_bytes=native.stat().st_size,trace_sha256=sha(native),native_only_admission_gap_cases=len(admissionCases),native_only_admission_gap_frames=len(admissionCases)*3,complete_native_trace_sha256=sha(fullNative),safety=safety,safety_checks=int(m[1]),allocation_failures=int(m[2]),native_log=nativeLog,owned_log=ownedLog,cpu_log=cpuLog,actual_weapon_refusal=weapon,executables=exes,reused_cpu=cpu,reused_multiply=old,multiply_regression=regression,source=sources,protected=pre['protected'],read_inputs={str(vmt):sha(vmt),**parent['read_inputs']},parent=pre['parent'],parent_sha256=pre['parent_sha256'],editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,static_sha256=sha(run/'static-reference.json'),query_sha256=sha(query),prepared_sha256=sha(prepared),limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','frames','init_refused','independent_checks','actual_chain_frames','preserved_vector4_count_frames','safety_checks','allocation_failures','exact_bytes','multiply_regression']},indent=2),flush=True)
if __name__=='__main__':main()
