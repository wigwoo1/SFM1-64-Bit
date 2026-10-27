"""Pinned client+IFM misses in real owned material lifetime and benchmark CPU draw."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,time,random
from verify_builtin_error_material_seed import source_closure
from verify_material_object_parameters import node,string,packet,text
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-pinned-proxy-profile-20261027'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');SESSION=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
MATERIAL=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
LIBS=ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_pinned_client_ifm_proxy_factory_candidate.h','test_material_pinned_proxy_profile.cpp','build_material_pinned_proxy_profile.cmd','verify_material_pinned_proxy_profile.py']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,message):
 if not ok:raise RuntimeError(message)
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(p)==h for p,h in pre['protected'].items()),'1207 protected entries before')
 need(sha(pre['parent'])==pre['parent_sha256']=='afd148f98ce69f09dd2eeddb9628151267022561109afbbf73c0f85b1882ff9a','accepted engine/tool routing receipt');parent=json.loads(Path(pre['parent']).read_text());client=json.loads(Path(parent['parent']).read_text())
 need(all(sha(OUT/'before'/Path(p).name)==h for p,h in pre['allowed_before'].items()),'exact two changed-source backups')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in NEW]);need([p for p,h in parent['source'].items() if sha(p)!=h]==list(pre['allowed_before']) or set(p for p,h in parent['source'].items() if sha(p)!=h)==set(pre['allowed_before']),'only two old sources changed')
 profile=(ROOT/'work'/NEW[0]).read_text();clientNames=[e['name'] for e in client['registrations']];ifmNames=[e['name'] for e in parent['ifm_interfaces']]
 def names(label):return re.findall(r'"([^"\n]+)"',re.search(label+r'\[\]=\{(.*?)\n \};',profile,re.S)[1])
 need(names('clientNames')==clientNames and names('ifmInterfaces')==ifmNames,'complete owned profile equals native-proven74/14 registration names')
 previous=Path('outputs/material-proxy-lifetime-20261026/verification-qwvmwmak/results.json');old=json.loads((ROOT/previous).read_text());need(sha(ROOT/previous)=='87b1cf17677ee5b97f209413449fa0a349013a4f584058056ad915ca9f28fabd','accepted caller and default safety baseline')
 need(all(sha(p)==h for p,h in old['private_library_hashes'].items()),'original private libraries')
 inputs=[SESSION,GAME/'usermod/gameinfo.txt',GAME/'usermod/models/weapons/w_models/w_flamethrower.mdl',GAME/'usermod/models/weapons/w_models/w_flamethrower.vvd',GAME/'usermod/models/weapons/w_models/w_flamethrower.dx90.vtx',GAME/'tf/materials/models/weapons/c_items/c_flamethrower.vmt',GAME/'tf/materials/models/weapons/c_items/c_flamethrower_blue.vmt',GAME/'tf/materials/models/effects/pyro/pilotlight.vmt'];readInputs={str(p):sha(p) for p in inputs};need(readInputs[str(SESSION)]=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','actual session identity')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 started=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_pinned_proxy_profile.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=300);(run/'build-driver.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'fresh isolated reference/profile/six regression builds')
 exes={};warnings={}
 stems={'caller':('caller_reference_x86','caller-native-build.log',0x14c),'profile':('pinned_proxy_profile_owned_x64','profile-build.log',0x8664)}
 for name in ['material_proxy_lifetime','unlit_two_texture_parameters','world_transition_parameters','lightmapped_parameters','material_object_parameters','material_store']:stems[name]=(name+'_owned_x64',name+'-build.log',0x8664)
 for tag,(stem,logName,machine) in stems.items():
  exe=OUT/(stem+'.exe');b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==machine and struct.unpack_from('<H',b,pe+24)[0]==(0x10b if machine==0x14c else 0x20b) and b'asInvoker' in b,'fresh architecture '+tag)
  log=(OUT/logName).read_text();(run/logName).write_text(log);found=re.findall(r'([^\r\n]+): warning (C\d+): ([^\r\n]+)',log);warnings[tag]=found
  if tag=='profile':need(len(found)==1 and found[0][1]=='C4458' and 'graph_transform_hierarchy.h(16)' in found[0][0], 'only unchanged hierarchy warning')
  else:need(not found,'clean regression/reference '+tag)
  need(not re.search(r'\berror C\d+',log),'no compile errors '+tag)
  inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspection);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.I|re.M);need(imports==['KERNEL32.dll'],'system-only direct imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=imports,fresh=True)
 def execute(tag,args,label):
  p=subprocess.run([exes[tag]['path'],*map(str,args)],cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+p.stdout+p.stderr);return p.stdout
 safety=execute('profile',[],'profile-safety');m=re.search(r'checks=(\d+) actual_allocation_failures=(\d+)',safety);need(m and int(m[1])>=997 and int(m[2])>=479,'actual init/rollback/ownership allocation coverage')
 implemented={'Equals','Sine','TextureTransform','TextureScroll','weapon_invis','Multiply','YellowLevel','ModelGlowColor'};lookup={n.lower():n for n in clientNames};namesToCheck=[];expected=[]
 for original in clientNames+ [n.removesuffix('_IMaterialProxy003') for n in ifmNames if n.endswith('_IMaterialProxy003')]+['invis','AnimatedWeaponSheen','Missing','', ' definitely_missing ', 'invis/extra']:
  for name in [original,original.lower(),original.upper(),''.join(c.upper() if i%2 else c.lower() for i,c in enumerate(original))]:
   namesToCheck.append(name);canonical=lookup.get(name.lower());expected.append(0 if canonical in implemented else 1 if canonical or name+'_IMaterialProxy003' in ifmNames else 2)
 rng=random.Random(202610274);namesToCheck.extend('absent_'+''.join(rng.choice('abcdefghijklmnopqrstuvwxyz') for _ in range(20)) for _ in range(500));expected.extend([2]*500);classify=run/'classify.txt';classify.write_text('\n'.join(namesToCheck)+'\n');classified=execute('profile',['--classify',classify],'classification');need([int(n) for n in classified.splitlines()]==expected,'complete registered/implemented/original-miss classification and exact IFM case behavior')
 # Real original material caller, controlled Bind services, and registry-known
 # Missing name. No built-in OnBind is simulated by these caller protocol spies.
 cases=[]
 for queued in [0,1]:
  for count in range(4):
   for missing in range(5):
    declarations=[node('Bind',[string('var','$TEMP' if i%2 else '$alpha'),string('tag',str(i))]) for i in range(count)]
    for i in range(missing):declarations.insert((i*2)%(len(declarations)+1),node('Missing'))
    root=node('UnlitTwoTexture',[string('$alpha','.375'),string('$temp','[.25 .5]'),node('Proxies',declarations)]);cases.append(dict(root=root,queued=queued,missing=missing,success=count))
 query=run/'caller-query.bin';query.write_bytes(struct.pack('<I',len(cases))+b''.join(struct.pack('<2I',0,c['queued'])+text('proxy/profile'+str(i))+packet(c['root']) for i,c in enumerate(cases)));mount=run/'mount';mount.mkdir();native=run/'caller-native.txt';owned=run/'caller-owned.txt';execute('caller',[MATERIAL,LIBS,mount,query,native],'caller-native');execute('profile',['--paired',query,owned],'caller-owned');need(native.read_bytes()==owned.read_bytes(),'actual original caller and owned policy full trace exact')
 for case,section in zip(cases,native.read_text().split('CASE ')[1:]):
  need(section.count('\nW 1\n')==case['missing']*2 and sum(line.startswith('D ') for line in section.splitlines())==case['success']*2,'missing warnings and survivor cleanup per two generations')
 benchmark=execute('profile',['--benchmark',SESSION,GAME],'actual-benchmark');need('c_flamethrower\tINITIALIZED\t2\t5' in benchmark and 'c_flamethrower_blue\tINITIALIZED\t2\t5' in benchmark and 'pilotlight\tINITIALIZED\t0\t7' in benchmark and 'actual_model=13817 CPU_draws=1 setters=11 rollback=PASS clock=EXPLICIT GPU=NOT_RUN' in benchmark,'full unchanged actual material declarations and model CPU transaction')
 regressions={}
 for tag,arg,key in [('material_proxy_lifetime','--test-proxies','proxies'),('unlit_two_texture_parameters','--test-unlit-two-texture','two'),('world_transition_parameters','--test-world-transition','world'),('lightmapped_parameters','--test-lightmapped','lightmapped'),('material_object_parameters','--test','parameters'),('material_store','--test','ownership')]:
  regressions[key]=execute(tag,[arg],tag+'-safety');need(regressions[key]==old['safety'][key],'exact unchanged previous safety counts '+key)
 oldRun=(ROOT/previous).parent;oldQuery=oldRun/'query.bin';need(sha(oldQuery)==old['query_sha256'],'accepted210-case default query');defaultTrace=run/'default-regression.txt';execute('material_proxy_lifetime',[run/'absent.dll',run/'absent-libraries',oldRun/'mount',oldQuery,defaultTrace],'default-caller-regression');need(sha(defaultTrace)==old['output_sha256'],'all210 old strict-default caller traces unchanged')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()) and all(sha(p)==h for p,h in readInputs.items()),'all current sources/protected/original read identities unchanged')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'complete editor313 dependency closure unchanged')
 limits=['INCOMPLETE independent x64 application; new explicitly selected pinned CLIENT+IFM static-service profile, no other/dynamic tool module claim. Default factories remain strict',
 'Owned profile carries74 original client names with case-insensitive lookup and14 original IFM interfaces with exact suffix matching. Eight existing owned implementations are available; other registered names refuse. Confirmed misses produce caller warnings with no fabricated instance and source declarations retained',
 'Known constructor Init failures still refuse atomically, even though original caller skips them. This is an explicit CPU/resource compatibility frontier, not a broad ignore-failures policy. No warning-state bypass of complete gate, ownership or pending scopes',
 'Actual original material caller executes with controlled Bind/FindVar/factory spies. Missing service behavior is separately grounded in previous engine/client/IFM registry proofs. No original complete joint engine/IFM/built-in shader frame claimed',
 'Actual untouched red/blue weapon materials initialize five active proxies with two missing warnings each; pilot initializes seven. Actual model13817 skin1/body0 selected blue/pilot CPU bridge executes11 setters and callback failure restores previous parameter pointers/values. Explicit time is supplied; native timeline clock mapping and GPU/texture/shader parity remain unverified',
 'One pre-existing hierarchy C4458 warning remains in new benchmark harness only; six regression builds and reference caller are clean. All are freshly compiled; previous baseline reference receipts reused for expected traces/counts, not mislabeled fresh native renderer execution',
 'No SFM/computer use, original writes, document/history/save/serialization/rendering changes, dependency install or publication. Complete editor and313 dependencies unchanged, so editor not rebuilt']
 result=dict(status='VERIFIED pinned material profile and actual benchmark CPU transaction; renderer/application INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',safety=safety,safety_checks=int(m[1]),allocation_failures=int(m[2]),classification_cases=len(expected),caller_cases=len(cases),caller_bytes=native.stat().st_size,caller_sha256=sha(native),benchmark=benchmark,regressions=regressions,default_regression_cases=old['cases'],executables=exes,compiler_warnings=warnings,source=sources,protected=pre['protected'],changed_sources=pre['allowed_before'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],read_inputs=readInputs,editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','safety_checks','allocation_failures','classification_cases','caller_cases','caller_bytes','benchmark','regressions']},indent=2),flush=True)
if __name__=='__main__':main()
