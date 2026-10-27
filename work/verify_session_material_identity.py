"""Bounded production resource-identity correction, independent benchmark checks.

Existing failing asset-dependent regressions must be recorded, not relabeled as
passes. The pristine before results/last editor are retained in the output folder.
No original file writes, SFM process, GPU test, publication or architecture changes.
"""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,shutil,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure

ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/material-resource-identity-20261027'
SESSION=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx')
AUDIT=ROOT/'outputs/material-find-cached-20261027/verification-aihox4s9/benchmark-audit.json'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
CHANGED=ROOT/'work/session_material_overrides_candidate.h'
NEW=['test_session_material_identity.cpp','build_session_material_identity.cmd','verify_session_material_identity.py']
REGRESSIONS=['test_session_material_overrides','test_scene_material_inputs','test_scene_draw_plan','test_selfillum_material_cache']

def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def need(b,s):
 if not b:raise RuntimeError(s)

def parse_audit(path):
 materials={};rejected={};owners={};payloads={};errors={}
 for line in path.read_text().splitlines():
  v=line.split('\t');tag=v[0]
  if tag=='M':need(len(v)==4,'material row');materials[int(v[1])]=dict(name=v[2],key=v[3])
  elif tag=='R':rejected[int(v[1])]=v[2]
  elif tag=='O':owners.setdefault(int(v[1]),[]).append(dict(position=int(v[2]),material=int(v[3])))
  elif tag=='P':payloads.setdefault(int(v[1]),[]).append(dict(position=int(v[2]),material=int(v[3]),key=v[4],parameters=int(v[5])))
  elif tag=='E':errors[int(v[1])]=v[2]
  else:raise RuntimeError('unknown audit row')
 return dict(materials=materials,rejected=rejected,owners=owners,payloads=payloads,errors=errors)

def main():
 before=json.loads((OUT/'before.json').read_text());parent=json.loads(Path(before['parent']).read_text())
 need(sha(before['parent'])==before['parent_sha256']=='35947bd438b530f559733b353eb6ead9e3a6db63c8d9d82a7ec035b65880b9bd','accepted historical receipt unchanged')
 allowed={str(CHANGED)};protected={p:h for p,h in before['protected'].items() if p not in allowed}
 need(all(sha(p)==h for p,h in protected.items()),'all unrelated original/source identities before')
 need(sha(OUT/'before/session_material_overrides_candidate.h')==before['protected'][str(CHANGED)],'preserved old header')
 old_frozen=ROOT/'outputs/material-find-uncached-20261027/frozen-6_ck0r2i'
 old_manifest=json.loads((old_frozen/'manifest.json').read_text())
 need(all(sha(old_frozen/p)==h for p,h in old_manifest.items()),'historical frozen source/receipt archive unchanged')
 need(sha(SESSION)==parent['benchmark_sha256'],'benchmark original unchanged')
 need(sha(AUDIT)==parent['benchmark_audit_sha256'],'native accepted all-graph audit')
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/p for p in NEW])
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_session_material_identity.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=240)
 (run/'focused-build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh focused/legacy builds')
 exes=[]
 for name in ['material_identity_x64',*REGRESSIONS]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
  need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'fresh AMD64 PE32+')
  log=OUT/('identity-build.log' if name=='material_identity_x64' else name+'-build.log');text=log.read_text()
  warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',text);need(not re.search(r'\berror C\d+',text),'no compile errors')
  need(all(code in ['C4996','C4458'] for code,_ in warnings),'only known transitive-header warnings')
  shutil.copyfile(log,run/log.name)
  inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout
  (run/(name+'-inspection.txt')).write_text(inspect)
  modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.M|re.I);need(modules==['KERNEL32.dll'],'owned tests system-only imports')
  exes.append(dict(name=name,path=str(exe),sha256=sha(exe),machine='AMD64 PE32+',warnings=warnings,imports=modules))
 testdir=run/'fixtures';testdir.mkdir();exe=OUT/'material_identity_x64.exe'
 p=subprocess.run([str(exe),'--self-test',str(testdir)],cwd=ROOT,capture_output=True,text=True,timeout=30)
 (run/'safety.log').write_text(p.stdout+p.stderr);need(p.returncode==0 and 'checks=37' in p.stdout,'focused37 checks')
 audit=run/'benchmark.tsv';p=subprocess.run([str(exe),'--audit',str(SESSION),str(audit)],cwd=ROOT,capture_output=True,text=True,timeout=90)
 (run/'benchmark.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'benchmark audit execution')
 need('names=236 rejected=0 arrays=107 positions=211 reader_errors=0' in p.stdout,'all graph arrays including3 empty')
 actual=parse_audit(audit);baseline=parse_audit(OUT/'before/benchmark.tsv');native=json.loads(AUDIT.read_text())
 native_materials={int(k):v for k,v in native['materials'].items()};native_owners={int(k):v for k,v in native['owners'].items()}
 need(actual['materials']==native_materials,'all236 material identities exactly match native-derived audit')
 need(actual['owners']==baseline['owners']==native_owners,'all104 nonempty arrays/211 positions/order retained')
 need(len(baseline['rejected'])==19 and len(baseline['errors'])==19,'before rejection evidence')
 need(set(baseline['rejected'])=={r['material'] for r in native['legacy_rejections']},'exact19 formerly rejected material IDs')
 need(not actual['rejected'] and not actual['errors'],'no name/parameter/schema rejections remain')
 need(len(actual['payloads'])==104 and sum(map(len,actual['payloads'].values()))==211,'typed payload checks across all referenced materials')
 for model,rows in actual['payloads'].items():
  need([dict(position=v['position'],material=v['material']) for v in rows]==native_owners[model],'source record order')
  need(all(v['key']==native_materials[v['material']]['key'] for v in rows),'correct resource key for every position')
 before_tests={r['name']:r for r in json.loads((OUT/'before/results.json').read_text())};regressions=[]
 for name in REGRESSIONS:
  p=subprocess.run([str(OUT/(name+'.exe'))],cwd=ROOT,capture_output=True,text=True,timeout=180)
  output=p.stdout+p.stderr;(run/(name+'-runtime.log')).write_text(output)
  need(p.returncode==before_tests[name]['returncode'] and output==before_tests[name]['summary'],'legacy result changed '+name)
  regressions.append(dict(name=name,returncode=p.returncode,unchanged_from_baseline=True,output=output,status='PASS' if p.returncode==0 else 'PRE-EXISTING FAILURE'))
 # Editor already rebuilt by the authoritative full script this session. Check
 # current artifacts, flags and the narrowly changed dependency closure; do not
 # waste another editor compile with identical source inputs.
 editor=Path(before['editor']);raw=editor.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
 need(struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'full editor AMD64 PE32+')
 need(sha(editor)!=before['editor_sha256'] and editor.stat().st_mtime>Path(OUT/'before/sfm64_session_ui.exe').stat().st_mtime,'editor rebuilt after backup')
 need(sha(OUT/'before/sfm64_session_ui.exe')==before['editor_sha256'],'last known editor preserved')
 deps={};visit=set()
 def walk(path):
  if path in visit:return
  visit.add(path);deps[path.relative_to(ROOT).as_posix()]=sha(path)
  for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',path.read_text(errors='replace'),re.M):
   child=path.parent/inc
   if child.exists():walk(child.resolve())
 walk(ROOT/'work/probe_vulkan_win32_surface_x64.cpp')
 changed_deps=[n for n,h in before['editor_dependencies'].items() if deps.get(n)!=h]
 added_deps=sorted(set(deps)-set(before['editor_dependencies']))
 need(changed_deps==['work/session_material_overrides_candidate.h'] and added_deps==['work/material_find_cache_key_candidate.h'],'only intended editor dependency transition')
 build_log=(editor.parent/'build.log').read_text();warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',build_log)
 baseline_warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(OUT/'before/build.log').read_text())
 need(warnings==baseline_warnings,'editor warning list unchanged')
 inspection=subprocess.run([str(DUMP),'/headers','/imports',str(editor)],capture_output=True,text=True,check=True).stdout
 (run/'editor-inspection.txt').write_text(inspection);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.M|re.I)
 need(not any(re.search('ifm|materialsystem|engine|tier0|vstdlib|filesystem_stdio',m,re.I) for m in modules),'no original SFM direct runtime imports')
 # This argument must fail during launch configuration, before window/Vulkan
 # initialization. No editor/SFM interaction or session loading is attempted.
 p=subprocess.run([str(editor),'--start-frame','0'],cwd=run,capture_output=True,text=True,timeout=30)
 (run/'editor-cli-smoke.log').write_text(p.stdout+p.stderr)
 need(p.returncode==1 and 'start frame requires a session' in p.stderr,'full editor no-window argument validation smoke')
 for name in ['build.log','build-manifest.txt','binary-headers.txt']:shutil.copyfile(editor.parent/name,run/('editor-'+name))
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in protected.items()),'all unrelated sources/inputs unchanged after')
 need(sha(SESSION)==parent['benchmark_sha256'],'original benchmark unchanged after')
 result=dict(status='VERIFIED bounded identity correction; PARTIALLY VERIFIED regression suite/application',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',
  before_rejected_materials=19,after_rejected_materials=0,benchmark_materials=236,benchmark_declared_material_arrays=107,
  benchmark_nonempty_material_arrays=104,benchmark_positions=211,exact_typed_parameter_records=211,safety_checks=37,
  regressions=regressions,executables=exes,parent=before['parent'],parent_sha256=before['parent_sha256'],
  intentional_changes={str(CHANGED):dict(before_sha256=before['protected'][str(CHANGED)],after_sha256=sha(CHANGED),reason='resource identity normalization delegates to already verified native cache-key helper; file resolvers unchanged')},
  source=sources,protected=protected,benchmark_sha256=sha(SESSION),benchmark_audit=str(audit),benchmark_audit_sha256=sha(audit),
  editor=str(editor),editor_sha256=sha(editor),editor_dependencies=deps,editor_rebuilt=True,editor_compiler_warnings=warnings,
  editor_imports=modules,editor_smoke='PASS no-window argument rejection; interactive/GPU runtime NOT RUN',
  limits=['all3 failing legacy regressions failed identically before change; no attempts to repair unrelated fixture/path/policy assumptions',
   'raw all-graph names/typed payloads, not active-shot/native-loaded graph or visual equivalence',
   'leading slash records intentionally remain unmatched to different slash-free model-material identities',
   'duplicate override refusal and existing lexical mount/asset path checks unchanged; no claim of broader filesystem resolver redesign/symlink handling',
   'prior native oracle receipts and frozen archives remain immutable; current production header intentionally differs from them',
   'shader/typed material lifetime/restoration and complete SFM compatibility remain incomplete'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
 print(json.dumps({k:result[k] for k in ['status','before_rejected_materials','after_rejected_materials','benchmark_materials','benchmark_positions','safety_checks','editor_sha256']},indent=2),flush=True)

if __name__=='__main__':main()
