"""Actual retained benchmark materials -> controlled CPU initialization audit.

This does not enable a renderer feature or discover the original active profile.
All outputs are workspace-only; no original process, DLL or GPU is invoked.
"""
import hashlib,json,re,shutil,struct,subprocess,tempfile,time
from datetime import datetime,timezone
from pathlib import Path
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/scene-material-parameter-bridge-20261027'
PARENT=ROOT/'outputs/material-puppet-context-transaction-20261027/verification-v75h2l15/results.json'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
SESSION=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
MOD=GAME/'usermod'
NEW=['probe_scene_material_parameter_bridge.cpp','build_scene_material_parameter_bridge.cmd','verify_scene_material_parameter_bridge.py']
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def main():
 OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 need(sha(PARENT)=='9dce2be958310c873cebbb0633a41f215d9bcd15533ca2c20d1cfb6f5445ab49','Accepted preceding CPU context receipt')
 prior=json.loads(PARENT.read_text());need(all(sha(n)==h for n,h in prior['sources'].items()),'All 395 preceding source files unchanged')
 exploratory=[json.loads(s) for s in (OUT/'exploratory.jsonl').read_text().splitlines()]
 protected={**prior['original_hashes'],**prior['benchmark_material_hashes']}
 # The installed DLL/config changed externally since the historical oracle
 # checkpoint. This run never executes that DLL. Record the mismatch rather
 # than silently treating it as the same original reference or restoring it.
 identity_changes=[]
 allowed_changes={str(GAME/'bin/tools/ifm.dll'):'12504c248047e181d31a8500fb6693d1a2d3c805e841fc2227e41aa4e2d2b47e',str(MOD/'gameinfo.txt'):'6d2c597cd5c74460c9c180d6b5c5fc4367b033337ed1e6a2a45e9d4dc55d6673'}
 for n,h in list(protected.items()):
  actual=sha(n)
  if actual!=h:
   need(n in allowed_changes and actual==allowed_changes[n],'Unexpected historical original identity change: '+n)
   identity_changes.append(dict(path=n,historical_sha256=h,current_sha256=actual));protected[n]=actual
 historical_ifm=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
 need(sha(historical_ifm)=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4','Preserved historical reference image')
 protected[str(historical_ifm)]=sha(historical_ifm)
 old=historical_ifm.read_bytes();current=(GAME/'bin/tools/ifm.dll').read_bytes();need(len(old)==len(current),'Installed IFM size changed again')
 diff=[i for i,(a,b) in enumerate(zip(old,current)) if a!=b]
 identity_audit=dict(changed_byte_count=len(diff),first_changed_file_offsets=[hex(i) for i in diff[:16]],historical_reference_matches_current_installation=False,original_runtime_invoked=False)
 for p in [SESSION,MOD/'gameinfo.txt',*[Path(r['path']) for r in exploratory if r['kind']=='read_source']]:protected[str(p)]=sha(p)
 need(sha(SESSION)=='5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee','Read-only benchmark identity')
 need(all(sha(n)==h for n,h in protected.items()),'Protected original source preflight')
 source=source_closure([ROOT/'work'/n for n in NEW]+[Path(n) for n in prior['sources']])
 # Publication rebuilt the default after the previous goal checkpoint. Pin
 # that actual present executable instead of asserting an obsolete old hash.
 default=ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe';default_before=sha(default)
 editor_status=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text())
 need(default_before==editor_status['executable_sha256'],'Current separately published complete editor')
 editor_deps=editor_status['editor_source_dependencies'];need(all(sha(ROOT/n)==h for n,h in editor_deps.items()),'Current complete editor source closure')
 start=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_scene_material_parameter_bridge.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240)
 (run/'build-driver.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'Fresh diagnostic compile')
 exe=OUT/'scene_material_bridge_owned_x64.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
 need(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b and b'asInvoker' in raw,'Fresh AMD64 PE32+ asInvoker diagnostic')
 header=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/'binary-inspection.txt').write_text(header)
 modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',header,re.M|re.I);need(modules==['KERNEL32.dll'],'No original SFM runtime imports')
 log=(OUT/'build.log').read_text();warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',log)
 need(len(warnings)==4 and all(c in {'C4458','C4996'} for c,_ in warnings),'Only four existing transitive header warnings')
 report=run/'actual-materials.jsonl';p=subprocess.run([str(exe),str(SESSION),str(MOD),str(report)],cwd=ROOT,capture_output=True,text=True,timeout=240)
 (run/'runtime.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'Actual retained scene diagnostic')
 rows=[json.loads(s) for s in report.read_text().splitlines()];materials=[r for r in rows if r['kind']=='material'];frames=[r for r in rows if r['kind']=='frame'];bindings=[r for r in rows if r['kind']=='cache_binding'];summary=[r for r in rows if r['kind']=='summary']
 need(len(summary)==1 and len(frames)==3 and [r['tick'] for r in frames]==[50000,50167,50333],'Three actual evaluated frames')
 need(len(materials)==30 and len(bindings)==15,'Five actual model materials per frame, two controlled profiles')
 need(len({(r['tick'],r['model'],r['slot'],r['profile']) for r in materials})==30,'Unique complete profile coverage')
 need(summary[0]['bindings']==len(materials) and summary[0]['frames']==len(frames),'Count framing')
 need(summary[0]['initialized']==sum(r['initialized'] for r in materials) and summary[0]['evaluated']==sum(r['evaluated'] for r in materials),'Actual initialized/evaluated accounting')
 need(summary[0]['refused']==sum(not r['evaluated'] for r in materials),'Refused count includes unresolved typed overrides')
 need(not summary[0]['profile_is_original_active_state'] and not summary[0]['gpu_or_visual_test'],'No active-profile or visual claim')
 known={'Sine','Equals','TextureTransform','TextureScroll','weapon_invis'}
 for r in materials:
  need(r['context_known']==(r['asset']!='*17'),'Actual model context category')
  need(r['unsupported_proxies']==[n for n in r['requested_proxies'] if n not in known],'Factory refusals explained by current registry')
  need(not r['resource_override'] or not r['evaluated'],'Untransported overrides must not become accepted effective materials')
  need(r['unchanged'] and not r['writes'],'Accepted no-proxy or refused diagnostic leaves parameters unchanged')
  need('diagnostic exception:' not in r['reason'],'No swallowed diagnostic failure')
 read_paths={Path(r['path']) for r in rows if r['kind']=='read_source'}
 protected_normalized={str(Path(n).resolve()).lower() for n in protected}
 need(all(str(n.resolve()).lower() in protected_normalized for n in read_paths),'All direct material-provider reads pinned before final run')
 cache={}
 for r in bindings:cache.setdefault(r['model'],[]).append(r)
 cache_findings=[]
 for model,entries in cache.items():
  identities={r['tick']:r['cpu_input_identity'] for r in entries};resources={(r['slot'],r['base_texture'],r['base_width'],r['base_height']) for r in entries}
  per_frame=[sorted((r['slot'],r['base_texture'],r['base_width'],r['base_height']) for r in entries if r['tick']==tick) for tick in [50000,50167,50333]]
  pixels={slot:{r['base_pixel_storage_identity'] for r in entries if r['slot']==slot} for slot in {r['slot'] for r in entries}}
  cache_findings.append(dict(model=model,input_identities_by_tick=identities,distinct_input_objects=len(set(identities.values())),same_base_texture_paths_and_dimensions=all(v==per_frame[0] for v in per_frame),base_pixels_bitwise_equal_to_first=all(r['base_pixels_equal_first'] for r in entries),distinct_base_pixel_allocations_by_slot={str(k):len(v) for k,v in pixels.items()},resources=sorted(resources)))
 need(all(sha(n)==h for n,h in protected.items()),'Original input hashes unchanged after diagnostic')
 need(all(sha(n)==h for n,h in source.items()),'Entire diagnostic/preceding source closure unchanged during run')
 need(sha(default)==default_before and all(sha(ROOT/n)==h for n,h in editor_deps.items()),'Complete editor and its 312 source dependencies preserved; no rebuild required')
 result=dict(status='VERIFIED diagnostic / INCOMPLETE application integration',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',parent=str(PARENT),parent_sha256=sha(PARENT),summary=summary[0],cache_findings=cache_findings,materials=[r for r in materials if r['tick']==50000],report=str(report),report_sha256=sha(report),executable=str(exe),executable_sha256=sha(exe),architecture='AMD64 PE32+',imports=modules,build_exit=build.returncode,runtime_exit=p.returncode,compiler_warnings=warnings,source=source,original_readonly_hashes=protected,historical_original_identity_changes=identity_changes,installed_ifm_comparison=identity_audit,editor_executable=str(default),editor_sha256=default_before,editor_dependencies=editor_deps,editor_rebuild='NOT REQUIRED: diagnostic-only source changes',gpu_visual='NOT RUN',limits=['Controlled profiles, not discovered original engine configuration','Historical IFM reference differs from current installed IFM; original behavior against modified install UNVERIFIED','Base VMT CPU initialization only when model overrides exist; no accepted effective-material claim','Only current three-frame benchmark model materials; appended world materials not individually CPU-initialized','Fresh per-binding diagnostic store, not a production retained lifecycle or history proof','No original runtime/oracle execution, GPU display, image comparison or full SFM compatibility claim'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
 print(json.dumps({k:result[k] for k in ['status','summary','cache_findings','executable_sha256','editor_rebuild']},indent=2),flush=True)
if __name__=='__main__':main()
