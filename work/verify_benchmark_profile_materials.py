"""Full read-only benchmark inventory, not a successful full-scene render."""
from pathlib import Path
from datetime import datetime,timezone
from collections import Counter,defaultdict
import hashlib,json,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/benchmark-profile-materials-20261027'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');SESSION=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['audit_benchmark_profile_materials.cpp','build_benchmark_profile_materials.cmd','verify_benchmark_profile_materials.py']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,message):
 if not ok:raise RuntimeError(message)
def main():
 pre=json.loads((OUT/'preflight.json').read_text());need(all(sha(p)==h for p,h in pre['protected'].items()),'1270 protected entries before')
 need(sha(pre['parent'])==pre['parent_sha256']=='845fb34051bd684fb3b899df81c5f8c7a1d9e994e268b7e02bb09e208fe7c186','accepted pinned profile receipt');parent=json.loads(Path(pre['parent']).read_text());sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in NEW]);need(all(sha(p)==h for p,h in parent['source'].items()),'no existing source changes')
 # Locate the accepted baseline from its pointer rather than guessing a run ID.
 oldPointer=json.loads((ROOT/'outputs/material-store-bindings-20261027/latest-verification.json').read_text());oldPath=Path(oldPointer['receipt']);need(sha(oldPath)==oldPointer['receipt_sha256']=='cc1c125294c314e216e32b447aec2378fd1956eefd8ad472e6a640d821145329','accepted actual binding audit');old=json.loads(oldPath.read_text());auditPath=oldPath.parent/'benchmark-audit.json';need(sha(auditPath)==old['benchmark_audit_sha256'],'accepted benchmark names/owners/reference state');baseline=json.loads(auditPath.read_text())
 need(sha(SESSION)=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','actual session identity')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_benchmark_profile_materials.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh owned x64 inventory build')
 exe=OUT/'benchmark_profile_materials_x64.exe';b=exe.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b and b'asInvoker' in b,'fresh AMD64 PE32+ asInvoker executable')
 log=(OUT/'inventory-build.log').read_text();(run/'inventory-build.log').write_text(log);warnings=re.findall(r'([^\r\n]+): warning (C\d+): ([^\r\n]+)',log);need(len(warnings)==1 and warnings[0][1]=='C4458' and 'graph_transform_hierarchy.h(16)' in warnings[0][0] and not re.search(r'\berror C\d+',log),'only unchanged hierarchy warning')
 inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/'inspection.txt').write_text(inspect);imports=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.I|re.M);need(imports==['KERNEL32.dll'],'owned Windows-only direct imports')
 trace=run/'inventory.tsv';p=subprocess.run([str(exe),str(SESSION),str(GAME),str(trace)],cwd=run,capture_output=True,text=True,timeout=300);(run/'inventory-run.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'complete audit execution: '+p.stdout+p.stderr)
 groups=defaultdict(list)
 for line in trace.read_text().splitlines():
  row=line.split('\t');groups[row[0]].append(row)
 materials={v[1]:dict(name=v[2],key=v[3]) for v in groups['M']};owners=defaultdict(list)
 for v in groups['O']:owners[v[1]].append(dict(position=int(v[2]),material=int(v[3])))
 need(materials==baseline['materials'] and dict(owners)==baseline['owners'],'all original236 names and107 owner arrays/211 positions unchanged')
 bindings={v[1]:dict(available=int(v[4]),resource=int(v[5])) for v in groups['M']}
 need(all(b=={k:baseline['bindings'][element][k] for k in b} for element,b in bindings.items()),'all217 acquired resources and19 unavailable bindings unchanged')
 session={int(v[1]):dict(name=v[2],initialized=bool(int(v[3])),references=int(v[4])) for v in groups['S']};allObjects={int(v[1]):dict(name=v[2],initialized=bool(int(v[3])),references=int(v[4])) for v in groups['T']};inits={int(v[1]):dict(name=v[2],root_shader=v[3],effective_shader=v[4],prepared=bool(int(v[5])),initialized=bool(int(v[6])),warnings=int(v[7]),proxies=int(v[8])) for v in groups['I']}
 need(len(session)==58 and len(allObjects)==197 and set(inits)==set(allObjects),'session cache versus additional model material resource counts')
 need(all(o['name']==baseline['objects'][str(id)]['name'] and o['references']==baseline['objects'][str(id)]['references'] for id,o in session.items()),'original cache names and per-element refcounts preserved')
 expectedRefs=Counter(b['resource'] for b in bindings.values() if b['available']);need(sum(expectedRefs.values())==217 and all(o['references']==expectedRefs[id] for id,o in allObjects.items()),'model temporary retains released;217 element retains remain')
 need(set(int(v[1]) for v in groups['R'])==set(allObjects) and all(v[2]=='0' for v in groups['R']),'all197 cache resources retire to0 references')
 need(sum(o['initialized'] for o in session.values())==19 and sum(o['initialized'] for o in allObjects.values())==52,'actual CPU initialization inventory')
 declarations=defaultdict(list)
 for v in groups['P']:declarations[int(v[1])].append(dict(name=v[2],classification=int(v[3]),initialized=int(v[4]),absent_configured_fields=v[5]))
 for id,r in inits.items():
  need(r['initialized']==allObjects[id]['initialized'],'actual published state matches init result')
  ds=declarations[id]
  if r['initialized']:need(r['prepared'] and all(d['classification']==2 or (d['classification']==0 and d['initialized']==1) for d in ds) and r['proxies']==sum(d['initialized']==1 for d in ds),'no unsupported registered effect silently accepted')
  if not r['prepared']:need(not ds and not r['initialized'],'shader/preparation failure happens before proxy construction')
  need(r['warnings']>=sum(d['classification']==2 for d in ds),'original-miss warnings recorded')
 batches={int(v[1]):dict(positions=int(v[2]),status=int(v[3]),observer_calls=int(v[4])) for v in groups['B']};need(len(batches)==107 and sum(v['positions'] for v in batches.values())==211,'all original batches audited')
 need(Counter(v['status'] for v in batches.values())==Counter({3:3,1:11,2:93}) and all(v['observer_calls']==int(v['status']==3) for v in batches.values()),'only complete batch observations; no passing nonempty override claim')
 draws={int(v[1]):dict(status=v[2],observer_calls=int(v[3]),writes=int(v[4]),asset=v[5],reason=v[6]) for v in groups['D']};need(set(draws)==set(batches),'each benchmark model gets a terminal disposition')
 need(Counter(v['status'] for v in draws.values())==Counter({'REFUSED_SAFE':103,'GEOMETRY_UNAVAILABLE':4}) and not groups['VISIT'] and all(v['observer_calls']==v['writes']==0 for v in draws.values()),'no actual override model claimed renderable; negative state/pointer guards exercised')
 selected=defaultdict(list)
 for v in groups['SELECT']:selected[int(v[1])].append(dict(texture=int(v[2]),resource=int(v[3]),initialized=bool(int(v[4]))))
 need([v['texture'] for v in selected[13817]]==[2,1] and all(v['initialized'] for v in selected[13817]),'real13817 selected blue/pilot CPU materials initialized')
 need(draws[13817]['reason']=='session binding unavailable','actual legacy session bindings prevent previously controlled draw')
 oldMissing=Counter(materials[element]['key'] for element,b in bindings.items() if not b['available']);need(oldMissing['models/weapons/w_flamethrower/w_flamethrower01_blue']==10 and oldMissing['models/weapons/w_flamethrower/w_flamethrower_hose']==5,'legacy flamethrower references unavailable in loose lookup')
 missingReads={v[1] for v in groups['L'] if v[2]=='0'};need('materials/models/weapons/w_flamethrower/w_flamethrower01_blue.vmt' in missingReads and 'materials/models/weapons/w_flamethrower/w_flamethrower_hose.vmt' in missingReads,'direct observed loose VMT misses, not invented native null resources')
 files={}
 for v in groups['F']:
  path=Path(v[1]);data=path.read_bytes();h=14695981039346656037
  for byte in data:h=((h^byte)*1099511628211)&0xffffffffffffffff
  need(len(data)==int(v[2]) and h==int(v[3]),'exact input bytes read by probe remain unchanged: '+str(path));files[str(path)]=hashlib.sha256(data).hexdigest()
 need(len(files)==267 and len(groups['PATH'])==50,'all267 read files and50 rooted material-path refusals recorded')
 unsupported=Counter();unsupportedResourceCounts=Counter();initFailures=[]
 for id,ds in declarations.items():
  unsupported.update(d['name'] for d in ds if d['classification']==1);unsupportedResourceCounts.update({d['name'] for d in ds if d['classification']==1})
  initFailures.extend(dict(resource=id,material=inits[id]['name'],**d) for d in ds if d['initialized']==0)
 need(unsupportedResourceCounts['AnimatedTexture']==125 and unsupportedResourceCounts['spy_invis']==83 and unsupportedResourceCounts['BurnLevel']==75,'measured high-impact registered effect gaps')
 prepareShaders=Counter(r['root_shader'] for r in inits.values() if not r['prepared']);need(prepareShaders==Counter({'EyeRefract':13,'Modulate':3,'Shadow':1}),'actual CPU shader preparation frontier')
 need(len(initFailures)==1 and initFailures[0]['name']=='TextureTransform' and initFailures[0]['absent_configured_fields']=='resultVar=$texture2transform;','observed Init refusal with absent configured result variable; no speculative no-op')
 regressions={}
 oldExecutable=parent['executables']['profile'];need(sha(oldExecutable['path'])==oldExecutable['sha256'],'exact accepted997-check profile executable');q=subprocess.run([oldExecutable['path']],cwd=run,capture_output=True,text=True,timeout=120);(run/'profile-regression.log').write_text(q.stdout+q.stderr);need(q.returncode==0 and 'checks=997 actual_allocation_failures=479' in q.stdout,'unchanged accepted profile safety');regressions['profile']=q.stdout
 bindingExecutable=old['executables']['owned'];need(sha(bindingExecutable['path'])==bindingExecutable['sha256'],'exact accepted binding safety executable');q=subprocess.run([bindingExecutable['path']],cwd=run,capture_output=True,text=True,timeout=120);(run/'binding-regression.log').write_text(q.stdout+q.stderr);need(q.returncode==0 and q.stdout.strip()==old['safety'],'unchanged actual binding ownership safety');regressions['bindings']=q.stdout
 need(all(sha(k)==h for k,h in sources.items()) and all(sha(k)==h for k,h in pre['protected'].items()) and all(sha(k)==h for k,h in files.items()),'all source/protected/original input identities after')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/k)==h for k,h in pre['editor_dependencies'].items()),'editor and313 dependencies unchanged')
 summary=dict(elements=236,bound=217,unavailable=19,session_resources=58,session_cpu_initialized=19,all_resources=197,all_cpu_initialized=52,arrays=107,positions=211,observed_empty_batches=3,unavailable_batches=11,value_refused_batches=93,cpu_draws=0,refused_draws=103,geometry_unavailable=4,rooted_path_refusals=50,element_retains=217,element_releases=217,read_files=267,read_bytes=sum(int(v[2]) for v in groups['F']))
 stringRows=[dict(model=int(v[1]),element=int(v[2]),name=v[3],type_flags=int(v[4]),target_exists=bool(int(v[5])),texture_request=v[6]) for v in groups['STRING']]
 textureBarrierModels=sorted({v['model'] for v in stringRows if v['target_exists']})
 allReadyLater=[id for id,d in draws.items() if d['reason']=='bridge metadata/path/context/value domain']
 knownTextureBarrier=[id for id in allReadyLater if id in textureBarrierModels]
 summary['later_gate_models_with_string_resource_rows']=len(knownTextureBarrier)
 detailed=dict(summary=summary,materials=materials,owners=dict(owners),bindings=bindings,session_objects=session,all_objects=allObjects,initializations=inits,declarations=declarations,batches=batches,draws=draws,selected=dict(selected),path_refusals=groups['PATH'],unavailable_names=dict(oldMissing),unsupported_declarations=dict(unsupported),unsupported_material_counts=dict(unsupportedResourceCounts),shader_preparation_refusals=dict(prepareShaders),proxy_init_failures=initFailures,string_override_rows=stringRows,models_with_bound_string_rows=textureBarrierModels,known_texture_barrier_models=knownTextureBarrier,read_inputs=files)
 (run/'inventory-audit.json').write_text(json.dumps(detailed,indent=2)+'\n')
 limits=['Independent x64 application remains INCOMPLETE. This is a benchmark-wide read-only inventory and controlled CPU transaction audit, not renderer/editor integration or original frame visual parity',
 'All107 model definitions with material arrays include inactive/hidden/other-shot content. Static session skin/body and explicit test time0x3e000000 used; no frame/fps/native clock or animated material-attribute equivalence claim',
 '236 session elements bind217 references to58 loose-file resources;19 of58 initialize. Model metadata lookup adds139 resources for197 total,52 CPU initialized. Resource cache existence is separate from initialized parameter/proxy state and renderability',
 'Actual scene override path completes0 model draws;103 model attempts refused before consumer with all current material parameter pointers/values/refcounts preserved;4 empty asset references unavailable. Preceding model13817 success used authored override rows, while actual scene rows name unavailable legacy materials. This audit preserves that difference',
 '50 rooted MDL material-name path requests rejected by existing host-relative path helper; recorded as compatibility frontier without stripping roots or substituting assets. Other missing loose assets may exist in VPK/dynamic mounts; complete native missing-material/error-resource behavior is unverified',
 'Recorded missing parameter fields on failed proxy Init are observations, not proof of every refusal cause. AnimatedTexture/spy_invis/BurnLevel/etc remain registered unimplemented; EyeRefract/Modulate/Shadow parameter preparation unavailable. No declaration or shader was removed to get passing counts',
 'First exploratory run stopped on rooted path exception; audit catch now records the unsupported request and continues with explicit unavailable tokens. Production loader/bridge behavior unchanged. No arbitrary path accepted or original resource modified',
 'One pre-existing hierarchy C4458 warning remains. New audit freshly compiled as AMD64 PE32+ with onlyKERNEL32.dll direct imports; accepted997/479 profile and binding safety executables reused by exact hash, not rebuilt',
 'No SFM/CUA/capture/window or process enumeration, original writes, session/history/save/serialization/rendering changes, new dependencies or publication. All preceding sources/1270 protected entries and313 editor dependencies unchanged; editor not rebuilt']
 result=dict(status='VERIFIED benchmark inventory and safe refusals; full scene CPU drawing/renderer INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',summary=summary,inventory_log=p.stdout,unsupported_material_counts=dict(unsupportedResourceCounts),shader_preparation_refusals=dict(prepareShaders),proxy_init_failures=initFailures,regressions=regressions,reused_executables={'profile':oldExecutable,'bindings':bindingExecutable},executable=dict(path=str(exe),sha256=sha(exe),machine='0x8664',imports=imports,fresh=True),compiler_warnings=warnings,audit_sha256=sha(run/'inventory-audit.json'),trace_sha256=sha(trace),source=sources,protected=pre['protected'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],read_inputs=files,editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','summary','unsupported_material_counts','shader_preparation_refusals','proxy_init_failures','regressions']},indent=2),flush=True)
if __name__=='__main__':main()
