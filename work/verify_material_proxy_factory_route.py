"""Original engine factory/tool fallback vs owned x64 dispatch; isolated IFM lookup."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,itertools,time
from pe_image import Image
from verify_ifm_draw_packet import check_ranges
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-proxy-factory-routing-20261027'
BIN=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin'
ENGINE=BIN/'engine.dll';IFM=BIN/'tools/ifm.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['material_proxy_factory_route_candidate.h','probe_material_proxy_factory_route.cpp','build_material_proxy_factory_route.cmd','verify_material_proxy_factory_route.py']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,message):
 if not ok:raise RuntimeError(message)
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(p)==h for p,h in pre['protected'].items()),'1179 protected entries before')
 need(sha(pre['parent'])==pre['parent_sha256']=='2055867f38c8e005fd61ed0e7a3ac7037ea558e4a4732efe098e3a7add813905','accepted client-registry receipt');parent=json.loads(Path(pre['parent']).read_text())
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/p for p in NEW])
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 engine,engineStatic=check_ranges(ENGINE,'engine-disassembly.txt',[('Create',0x1019d580,0x1019d5f2),('Delete',0x1019d600,0x1019d615),('tools_enabled',0x101e6680,0x101e6698),('tool_loop',0x101e6ac0,0x101e6b06),('material_system_factory_install',0x100d0743,0x100d0758)])
 need(engine.word(0x104010f8)==0x1033ffe0 and engine.word(0x1033ffe0)==0x1019d580 and engine.word(0x1033ffe4)==0x1019d600,'engine factory object/vtable')
 need(engine.word(0x1040c360)==0x1040c37c and engine.word(0x1040c37c)==0x10366bfc and engine.word(0x10366bfc+41*4)==0x101e6680 and engine.word(0x10366bfc+44*4)==0x101e6ac0,'real tool-system route')
 ifm=Image(IFM);callbacks=[]
 for line in (ROOT/'work/ifm-disassembly.txt').open():
  m=re.match(r'\s*([0-9A-F]{8}):\s+E8 (?:[0-9A-F]{2} ){4}\s*call\s+10980B80\s*$',line)
  if m:callbacks.append(int(m[1],16)-15)
 need(len(callbacks)==14,'complete IFM InterfaceReg direct-constructor callback set')
 entries=[]
 for callback in callbacks:
  o=ifm.offset(callback);b=ifm.b[o:o+21];need(b[0]==0x68 and b[5]==0x68 and b[10]==0xb9 and b[15]==0xe8 and b[20]==0xc3,'IFM registration shape')
  name=ifm.word(callback+1);creator=ifm.word(callback+6);obj=ifm.word(callback+11)
  refs=[ifm.base+r+m.start() for r,n,o in ifm.sections for m in re.finditer(re.escape(struct.pack('<I',callback)),ifm.b[o:o+n])];need(len(refs)==1,'single static IFM initializer pointer')
  entries.append(dict(callback=callback,object=obj,creator=creator,name_address=name,name=ifm.string(name),initializer=refs[0]))
 entries.sort(key=lambda e:e['initializer']);need(sum(e['name'].endswith('_IMaterialProxy003') for e in entries)==13,'13 original IFM proxy interfaces plus tool interface')
 _,ifmStatic=check_ranges(IFM,'ifm-disassembly.txt',[('InterfaceReg_constructor',0x10980b80,0x10980ba2),('CreateInterface',0x10980bb0,0x10980c1e),('factory_accessor',0x10980c70,0x10980c76),('tool_proxy_name_bridge',0x102bc2c0,0x102bc331)]+[(e['name'],e['callback'],e['callback']+21) for e in entries])
 need(ifm.string(0x10ae761c)=='_IMaterialProxy003' and ifm.word(0x10aeb09c)==0x102bc2c0,'actual IFM proxy suffix and virtual method pointer')
 _,materialStatic=check_ranges(BIN/'materialsystem.dll','materialsystem-disassembly.txt',[('material_proxy_Init_loop',0x10002c50,0x10002dcd),('material_proxy_Release',0x10002dd0,0x10002e2d)])
 oldPointer=json.loads((ROOT/'outputs/material-proxy-lifetime-20261026/latest-verification.json').read_text());oldReceipt=Path(oldPointer['receipt']);need(sha(oldReceipt)==oldPointer['receipt_sha256']=='87b1cf17677ee5b97f209413449fa0a349013a4f584058056ad915ca9f28fabd','accepted original caller lifetime receipt')
 replay=Path(oldPointer['replay']);need(sha(replay/'manifest.json')==oldPointer['replay_manifest_sha256']=='7cd8d274425732da931b6b60f849a663f63074965386b8b877c274b5f094bbe1','accepted caller replay manifest');manifest=json.loads((replay/'manifest.json').read_text());boundary=oldReceipt.parent/'native-boundaries.txt';boundaryName=boundary.relative_to(ROOT).as_posix();need(sha(boundary)==manifest[boundaryName]==sha(replay/boundaryName),'exact original missing/failed-Init trace in accepted replay')
 mixed=boundary.read_text().split('CASE 0\n')[1].split('CASE 1\n')[0];need('C 2 Missing\nW 1\nC 3 Fail\n' in mixed and 'D 3 1 0\nW 2\nC 4 Bind\n' in mixed and 'D 4 0 0\nD 1 0 0\n' in mixed,'original caller warns/skips missing and failed Init; survivors retain order/reverse release')
 callerEvidence=dict(receipt=str(oldReceipt),receipt_sha256=sha(oldReceipt),boundary_trace=str(boundary),boundary_sha256=sha(boundary),freshly_executed=False)
 (run/'static-reference.json').write_text(json.dumps(dict(engine=engineStatic,ifm=ifmStatic,material=materialStatic),indent=2)+'\n');(run/'ifm-registrations.json').write_text(json.dumps(entries,indent=2)+'\n')
 registrationQuery=run/'ifm-registrations.tsv';registrationQuery.write_text(''.join(f"{e['callback']:x}\t{e['object']:x}\t{e['creator']:x}\t{e['name_address']:x}\n" for e in entries))
 # A deliberate, exhaustive bounded service-state cross product. Native methods
 # determine client priority, enabled-query order, first tool hit and warnings.
 cases=list(itertools.product([0,1],[0,1,2,255],[0,1,2,255],[0,1,2,255],[-1,0,1,2,4],[-1,0,1,3,4],['invis','AnimatedWeaponSheen','ModelGlowColor','']))
 query=run/'queries.tsv';query.write_text(''.join(' '.join(map(str,c[:6]))+' '+c[6]+'\n' for c in cases));need(len(cases)==12800,'bounded case cardinality')
 # One-command verifier must prove its own fresh compile on later reruns too.
 started=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_material_proxy_factory_route.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'fresh original-reference and owned x64 builds')
 exes={}
 for tag,name,machine,magic in [('native','proxy_factory_route_reference_x86',0x14c,0x10b),('owned','proxy_factory_route_owned_x64',0x8664,0x20b)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
  need(exe.stat().st_mtime>=started-2 and all(exe.stat().st_mtime>=(ROOT/'work'/n).stat().st_mtime for n in NEW),'fresh current-run compile after source edits '+tag)
  need(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic and b'asInvoker' in raw,'architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error) C\d+',log),'clean compile '+tag)
  inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspection);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.I|re.M);need(imports==['KERNEL32.dll'],'system-only direct imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=imports,fresh_compile_this_turn=True)
 def execute(tag,args,label):
  p=subprocess.run([exes[tag]['path'],*map(str,args)],cwd=run,capture_output=True,text=True,timeout=120);(run/(label+'.txt')).write_text(p.stdout);(run/(label+'.stderr.txt')).write_text(p.stderr);need(p.returncode==0,label+' exit='+str(p.returncode)+' '+p.stderr);return p.stdout
 native=execute('native',['--engine',ENGINE,query],'native-engine');owned=execute('owned',[query],'owned-route');need(native==owned,'whole engine Create and tool loop match actual x64 component')
 rows=[line.split('\t') for line in native.splitlines() if line.startswith('CASE\t')];need(len(rows)==len(cases),'all traces recorded');checks=0;fallbackHits=misses=0
 for index,(c,row) in enumerate(zip(cases,rows)):
  client,local,global_,suppress,count,hit,name=c;toolUsed=not client and (local or global_);hitIndex=hit if toolUsed and 0<=hit<count else -1;token=client or (2+hitIndex if hitIndex>=0 else 0);warn=not token and not suppress
  order=['C','E']
  if toolUsed:order+=['N']+['T'+str(i) for i in range(hitIndex+1 if hitIndex>=0 else max(count,0))]
  if warn:order+=['W']
  need(row==['CASE',str(index),str(token),str(int(warn)),' '.join(order)+' '],'independent priority/ordered loop/warning check');checks+=1;fallbackHits+=hitIndex>=0;misses+=not token
 ifmLog=execute('native',['--ifm',IFM,registrationQuery],'native-ifm');need('PASS ifm_registrations=14 checks=43 bridge_hits=13' in ifmLog and 'IFM_MISSING\tinvis_IMaterialProxy003' in ifmLog and 'IFM_MISSING\tAnimatedWeaponSheen_IMaterialProxy003' in ifmLog,'complete IFM missing-name and positive bridge coverage')
 safety=execute('owned',[],'owned-safety');need('PASS owned_route_safety=18' in safety,'unknown/throwing service and input-domain refusals')
 old=parent['reused_owned'];need(sha(old['path'])==old['sha256'],'previous owned material safety identity');p=subprocess.run([old['path']],cwd=run,capture_output=True,text=True,timeout=120);(run/'material-regression.txt').write_text(p.stdout+p.stderr);need(p.returncode==0 and 'checks=48 actual_allocation_failures=14' in p.stdout,'unchanged old material admission safety')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()),'all sources/protected inputs unchanged after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 limits=['INCOMPLETE independent x64 application; new owned CPU route is an opt-in service boundary, not complete material/proxy/renderer admission',
 'Original engine Create1019d580 always asks client then tools-enabled, even on client hit; only enabled client misses enter original tools loop101e6ac0, which returns first nonnull result in current list order. Warning suppress byte tested as any nonzero value',
 'Native engine uses explicit host client/tool leaf/count/warning spies and actual original tools-enabled helper and tool loop; direct code unmodified RX. No original client/IFM/engine process simultaneously linked or live application run',
 'Original IFM14 static InterfaceReg callbacks and actual interface list walker replayed;13 proxy names. Full original IFM suffix/lookup bridge exercised with explicit replacement creator pointers in registry DATA, without constructing unknown real shader objects. Exact original names and linked list retained. Missing names invoke no creator',
 'Client registry preceding receipt plus IFM local registry miss prove invis/AnimatedWeaponSheen unavailable through those pinned local services. Other dynamically loaded tool modules, registration changes and full engine startup composition remain unverified; never treat missing owned code as original unregistered behavior',
 'Owned nullopt service state refuses; present token0 is an actual consulted-services miss. ASCII strings<=255 without embedded NUL and toolCount<=4096 admitted. Native invalid/null pointer/huge list/locale domains not reproduced. Returned tokens are borrowed; no proxy ownership or arbitrary provider-effect rollback',
 'No changes to strict old factory/store material initialization, document/history/serialization/rendering, assets, dependencies, editor launch, original writes or publication. Complete weapon VMTs remain safely refused pending policy integration. Preceding x64 material regression reused by exact hash, not rebuilt; editor313 dependencies unchanged']
 result=dict(status='VERIFIED bounded original engine/tool route and owned x64 dispatch; full material/application INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',cases=len(cases),independent_checks=checks,fallback_hit_cases=fallbackHits,miss_cases=misses,trace_sha256=sha(run/'native-engine.txt'),trace_bytes=(run/'native-engine.txt').stat().st_size,ifm_interfaces=entries,ifm_log=ifmLog,original_material_caller=callerEvidence,safety=safety,material_regression=p.stdout,executables=exes,source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],read_inputs=pre['read_inputs'],editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','independent_checks','fallback_hit_cases','miss_cases','trace_bytes','safety','ifm_log','material_regression']},indent=2),flush=True)
if __name__=='__main__':main()
