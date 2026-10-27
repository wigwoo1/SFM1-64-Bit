"""Test-only fixture/configuration maintenance; owned and real-asset runs separate."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,shutil,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure

ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-test-fixtures-20261027'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');CONTENT=GAME/'usermod'
SESSION=ROOT/'work/roundtrip-11.dmx';BENCHMARK=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NAMES=['test_session_material_overrides','test_scene_material_inputs','test_selfillum_material_cache']
WRAPPERS=['build_session_material_overrides_test.cmd','build_scene_material_inputs_test.cmd','build_selfillum_material_cache.cmd']
NEW=['material_test_fixtures.h','build_material_regression_fixtures.cmd','verify_material_regression_fixtures.py']

def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def need(b,s):
 if not b:raise RuntimeError(s)

def main():
 before=json.loads((OUT/'before.json').read_text());parent=json.loads(Path(before['parent']).read_text())
 need(sha(before['parent'])==before['parent_sha256']=='eaae97be2076349b9ba71cd129d3104719b39e18679933fc92465152d153032b','accepted receipt')
 allowed=set(before['intended_changes']);protected={p:h for p,h in before['protected'].items() if p not in allowed}
 need(all(sha(p)==h for p,h in protected.items()),'unrelated source/inputs before')
 for p in allowed:
  old=OUT/'before'/Path(p).name
  if p in before['protected']:need(sha(old)==before['protected'][p],'old test/script retained')
 # Retain frozen prior evidence; never relabel a historical failed test as passed.
 frozen=ROOT/'outputs/material-resource-identity-20261027/frozen-5fqoo6a0';manifest=json.loads((frozen/'manifest.json').read_text())
 need(all(sha(frozen/p)==h for p,h in manifest.items()),'previous frozen evidence unchanged')
 original_inputs={str(SESSION):sha(SESSION),str(BENCHMARK):sha(BENCHMARK),str(CONTENT/'gameinfo.txt'):sha(CONTENT/'gameinfo.txt')}
 for folder in [CONTENT/'models/anno/mamagen',CONTENT/'materials/models/anno/mamagen',CONTENT/'materials/models/sneakyerz/wigwoo1/hobkin']:
  for p in folder.rglob('*'):
   if p.is_file() and p.suffix.lower() in ['.mdl','.vvd','.vtx','.vmt','.vtf']:original_inputs[str(p)]=sha(p)
 need(original_inputs[str(BENCHMARK)]==parent['benchmark_sha256'],'benchmark identity')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/p for p in NEW+WRAPPERS+[n+'.cpp' for n in NAMES]])
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 results=[];start=time.time()
 def execute(command,label,expected=0,timeout=180):
  p=subprocess.run(list(map(str,command)),cwd=ROOT,capture_output=True,text=True,timeout=timeout)
  output=p.stdout+p.stderr;(run/(label+'.log')).write_text(output)
  need(p.returncode==expected,label+' exit '+str(p.returncode)+' '+output[-1000:])
  result=dict(label=label,command=list(map(str,command)),returncode=p.returncode,output=output);results.append(result);return output
 # Actual legacy entry scripts must compile and forward runtime arguments only.
 for name,wrapper in zip(NAMES,WRAPPERS):
  directory=run/(name+'-owned');directory.mkdir()
  output=execute(['cmd.exe','/d','/c',ROOT/'work'/wrapper,'--fixtures',directory],name+'-owned')
  need('PASS' in output,'owned pass marker')
 execute(['cmd.exe','/d','/c',ROOT/'work/build_material_regression_fixtures.cmd','--support-only'],'support-build')
 executables=[]
 for name in NAMES+['test_scene_draw_plan','test_session_material_identity']:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
  need(exe.stat().st_mtime>=start-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'fresh x64 test '+name)
  log=OUT/(name+'-build.log');text=log.read_text();warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',text)
  need(all(c in ['C4996','C4458'] for c,_ in warnings) and not re.search(r'error C\d+',text),'known transitive warnings only')
  shutil.copyfile(log,run/log.name)
  inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout
  (run/(name+'-inspection.txt')).write_text(inspection);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.M|re.I)
  need(modules==['KERNEL32.dll'],'no original/GPU direct imports')
  executables.append(dict(name=name,path=str(exe),sha256=sha(exe),machine='AMD64 PE32+',warnings=warnings,imports=modules))
 override=execute([OUT/(NAMES[0]+'.exe'),'--integration',SESSION,CONTENT],'override-integration')
 scene=execute([OUT/(NAMES[1]+'.exe'),'--integration',SESSION,CONTENT],'scene-integration')
 glow=execute([OUT/(NAMES[2]+'.exe'),'--integration',CONTENT],'selfillum-integration')
 need('base_parameters=7' in override and 'override_texture_resolved=5 missing=2' in override,'integration override coverage and explicit missing files')
 need(scene.count('packed_scene vertices=47850 indices=198438')==3 and scene.count('saved_texture_overrides=4')==3,'preserved real geometry and override assertions')
 need('entries=3 hits=4' in scene and 'entries=4 hits=2' in glow,'real cache fallback/restore counts')
 gameinfo=(CONTENT/'gameinfo.txt').read_text();block=re.search(r'SearchPaths\s*\{([^}]*)\}',gameinfo,re.S)
 need(block is not None,'installed explicit SearchPaths block');declared=re.findall(r'^\s*Game\s+([^\r\n]+)',block[1],re.M|re.I)
 need(len(declared)==6 and 'integration_game_mounts=12' in override,'six declared/12 derived current paths; no14-mount assertion')
 draw=execute([OUT/'test_scene_draw_plan.exe'],'draw-plan');need('passed' in draw,'draw regression')
 identity_directory=run/'identity-fixtures';identity_directory.mkdir()
 identity=execute([OUT/'test_session_material_identity.exe','--self-test',identity_directory],'identity-safety')
 need('checks=37' in identity,'identity37 retained')
 audit=run/'benchmark.tsv';benchmark=execute([OUT/'test_session_material_identity.exe','--audit',BENCHMARK,audit],'benchmark-audit')
 need('names=236 rejected=0 arrays=107 positions=211 reader_errors=0' in benchmark,'benchmark compatibility unchanged')
 # Compare all material/owner/payload rows with the accepted prior production run.
 old=Path(parent['benchmark_audit']).read_bytes();need(audit.read_bytes()==old,'entire benchmark identity/order/payload audit unchanged')
 for i,name in enumerate(NAMES):
  args=[OUT/(name+'.exe'),'--integration']
  if i<2:args.extend([run/'absent-session.dmx',run/'absent-content'])
  else:args.append(run/'absent-content')
  output=execute(args,name+'-missing-fixture',expected=1)
  need('Integration fixture unavailable:' in output,'readable missing-fixture error')
  output=execute([OUT/(name+'.exe')],name+'-usage',expected=1);need('Use --fixtures' in output,'readable test CLI usage')
  # Reject workspace-root reuse and already-populated directories before writes.
  output=execute([OUT/(name+'.exe'),'--fixtures',ROOT],name+'-unsafe-fixture',expected=1)
  need('workspace outputs' in output,'fixture writes constrained to workspace outputs')
  output=execute([OUT/(name+'.exe'),'--fixtures',run/(name+'-owned')],name+'-reused-fixture',expected=1)
  need('fresh empty workspace fixture' in output,'no overwrite of existing test files')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'unrelated source/inputs unchanged after')
 need(all(sha(p)==h for p,h in original_inputs.items()),'all pinned private fixture assets/session unchanged after')
 need(sha(before['editor'])==before['editor_sha256'] and all(sha(ROOT/p)==h for p,h in before['editor_dependencies'].items()),'editor313 dependencies unchanged; no rebuild required')
 for e in executables:need(sha(e['path'])==e['sha256'],'tested executable stable')
 changes={p:dict(before_sha256=sha(OUT/'before'/Path(p).name),after_sha256=sha(p),scope='test or test build script') for p in allowed}
 result=dict(status='VERIFIED test-fixture maintenance; application fidelity remains PARTIALLY VERIFIED',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',
  owned_fixture_runs=3,real_asset_integration_runs=3,negative_cli_checks=12,controlled_gameinfo_checks=5,
  actual_game_paths=6,derived_game_paths=12,custom_texture_overrides_resolved=5,custom_texture_overrides_missing=2,
  real_scene_frames=3,real_scene_vertices=47850,real_scene_indices=198438,identity_safety_checks=37,benchmark_materials=236,benchmark_positions=211,
  source=sources,protected=protected,original_inputs=original_inputs,intentional_changes=changes,executables=executables,
  parent=before['parent'],parent_sha256=before['parent_sha256'],results=results,
  benchmark_audit=str(audit),benchmark_audit_sha256=sha(audit),benchmark_sha256=sha(BENCHMARK),
  editor=before['editor'],editor_sha256=before['editor_sha256'],editor_dependencies=before['editor_dependencies'],editor_rebuilt=False,
  limits=['Tests now verify current explicit owned missing-texture/mask fallback data; native renderer parity not established',
   'Real integration requires local private roundtrip session/custom models; public source uses authored VMT/VTF/geometry/Graph fixture mode separately',
   'Two authored custom textures remain missing, explicitly diagnosed; no claim that every installed asset resolves',
   'No production/editor source, serialization, rendering, animation or filesystem-policy changes; only tests/fixtures/build scripts',
   'No SFM process, computer use, GPU rendering, original writes, installed config changes or publication',
   'Complete native64 SFM compatibility and visual/lifetime correctness remain incomplete'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
 print(json.dumps({k:result[k] for k in ['status','owned_fixture_runs','real_asset_integration_runs','negative_cli_checks','real_scene_frames','identity_safety_checks','benchmark_materials']},indent=2),flush=True)

if __name__=='__main__':main()
