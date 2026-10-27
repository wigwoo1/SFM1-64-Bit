"""Owned x64 prebind selection vs whole original helper; no renderer admission."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile,time
from verify_builtin_error_material_seed import source_closure
from verify_ifm_draw_packet import check_ranges
from verify_model_proxy_data_route import corpus,decode,expected
ROOT=Path(__file__).resolve().parent.parent;OUT=ROOT/'outputs/studio-proxy-prebind-owned-20261027'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
STUDIO=GAME/'bin/studiorender.dll';ENGINE=GAME/'bin/engine.dll'
MATERIAL=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/materialsystem.dll'
IFM=ROOT/'outputs/material-service-provider-scan-20261024/copied-bin/tools/ifm.dll'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['studio_proxy_prebind_selection_candidate.h','test_studio_proxy_prebind_selection.cpp','build_studio_proxy_prebind_selection.cmd','verify_studio_proxy_prebind_selection.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def main():
 pre=json.loads((OUT/'preflight.json').read_text());parent=json.loads(Path(pre['parent']).read_text());route=json.loads(Path(pre['route']['receipt']).read_text())
 need(sha(pre['parent'])==pre['parent_sha256']=='077b825c84a626f505ccac5c337fb8eeeedfbb53a06af7b22196036ca38f025a','accepted preceding clock receipt')
 need(sha(pre['route']['receipt'])==pre['route']['receipt_sha256']=='820c7696785433ea06406c4a622c1b3307f6ce29395806a2127d66f11a35926b','accepted earlier route receipt')
 need(all(sha(p)==h for p,h in pre['protected'].items()),'preceding protected closure before')
 frozen=Path(pre['route']['replay']);need(sha(frozen/'manifest.json')==pre['route']['replay_manifest_sha256']=='7ba16b20c324c6fa0952a2eb02b519fb23c9f84ef79633544cf5a6c08046a281','earlier route frozen manifest');need(all(sha(frozen/n)==h for n,h in json.loads((frozen/'manifest.json').read_text()).items()),'earlier frozen evidence unchanged')
 for n in ['probe_model_proxy_data_route.cpp','native_private_world_image_fixture.h']:need(sha(ROOT/'work'/n)==route['sources'][str(ROOT/'work'/n)],'exact accepted whole-method fixture source')
 inputs={str(STUDIO):'dc7eef7620f2a687c3cdd4182f2f17f0d78890e2748abcbe923b459b5fa340f7',str(ENGINE):'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',str(MATERIAL):'c78e414891b4d87e3922610a8bf937bca3efbaab95bced488522045998bc87f8',str(IFM):'8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'}
 need(all(sha(p)==h for p,h in inputs.items()),'original image identities')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/n for n in NEW]+[ROOT/'work/probe_model_proxy_data_route.cpp'])
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 # Recheck all earlier route spans against actual image bytes, not just a saved
 # claim. This preserves its static-vs-executed boundary and closes the normal
 # IFM->VEngineModel->StudioRender interface/data route in one current receipt.
 static={}
 for kind,path,dump in [('studio',STUDIO,'studiorender-disassembly.txt'),('engine',ENGINE,'engine-disassembly.txt'),('material',MATERIAL,'materialsystem-disassembly.txt')]:
  ranges=[(q['name'],int(q['start'],16),int(q['end_exclusive'],16)) for q in route['original_static_ranges'][kind]]
  if kind=='engine':ranges += [('interface_factory_binding_loop',0x10239640,0x1023968b),('model_slot16',0x100d69f0,0x100d6ae6),('model_slot18',0x100d67d0,0x100d683f),('complete_model_slot19_static',0x100dabe0,0x100db254),('client_time_accessor_static',0x10090160,0x100901c2),('client_global_view_static',0x100832d0,0x100832d9),('client_time_predicate_static',0x1008f7e0,0x1008f833)]
  image,static[kind]=check_ranges(path,dump,ranges)
  if kind=='engine':need(image.word(0x1040e9b8)==0x103614b4 and image.string(0x103614b4)=='VStudioRender026' and image.word(0x1040e9bc)==0x106941d4,'actual StudioRender interface acquisition')
  if kind=='studio':need(image.word(0x100710dc+28*4)==0x10057340,'actual StudioRender draw slot28')
 _,static['ifm']=check_ranges(IFM,'ifm-disassembly.txt',[('override_apply_call',0x102438a6,0x102438c2),('normal_model_draw',0x10243a57,0x10243b8c),('backup_replay_call',0x10243fca,0x10243fe3),('VEngineModel_acquisition',0x102baff0,0x102bb00e)])
 (run/'static-route.json').write_text(json.dumps(static,indent=2))
 started=time.time();build=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_studio_proxy_prebind_selection.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(build.stdout+build.stderr);need(build.returncode==0,'fresh reference and owned builds')
 exes={}
 for tag,name,machine in [('reference','prebind_reference_x86',0x14c),('owned','prebind_owned_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==machine and b'asInvoker' in raw,'fresh executable architecture/manifest '+tag)
  log=(OUT/(tag+'-build.log')).read_text();(run/(tag+'-build.log')).write_text(log);need(not re.search(r'\b(?:warning|error)\b',log,re.I),'clean compile '+tag)
  inspection=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/(tag+'-inspection.txt')).write_text(inspection);modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspection,re.I|re.M);need(modules==['KERNEL32.dll'],'direct imports '+tag);exes[tag]=dict(path=str(exe),sha256=sha(exe),machine=hex(machine),imports=modules)
 def execute(exe,args,label):
  p=subprocess.run([str(exe)]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+p.stdout+p.stderr);return p.stdout.strip()
 safety=execute(exes['owned']['path'],[],'owned-safety');m=re.search(r'checks=(\d+) allocation_failures=(\d+)',safety);need(m and int(m[1])>20 and int(m[2])>1,'owned validation/allocation coverage')
 cases=corpus()['studio'];query=run/'queries.bin';query.write_bytes(b''.join(struct.pack('<9I',*q) for q in cases));need(sha(query)==route['routing']['studio']['query_sha256'],'exact accepted corpus')
 native_log=execute(exes['reference']['path'],[STUDIO,query,run/'native.bin'],'native');need(sha(run/'native.bin')==route['routing']['studio']['output_sha256']=='938f39c9b0cde195b46f879f23298b02c602adc89da2f6bfa8384979bba05c35','all earlier whole original helper observations preserved')
 owned_log=execute(exes['owned']['path'],['--wire',query,run/'owned.bin'],'owned-wire');need((run/'owned.bin').read_bytes()==(run/'native.bin').read_bytes(),'entire owned selection trace equals original helper')
 records=decode((run/'native.bin').read_bytes());need(len(records)==len(cases)==1636,'all cases observed');events=0
 for i,(q,actual) in enumerate(zip(cases,records)):
  final,wanted=expected('studio',q);need(actual==(i,final,wanted),'original and owned vs separate sequential model');events+=len(wanted)
 need(events==4722,'exact eligibility/bind events')
 # Retain the already verified immediate-path corpus as reused evidence. Its
 # method is unchanged; do not rebuild/overwrite old accepted output files.
 immediate=next(e for e in route['executables'] if Path(e['path']).name=='immediate_route_reference_x86.exe');need(sha(immediate['path'])==immediate['sha256'],'exact earlier immediate executable')
 priorRun=Path(pre['route']['receipt']).parent;immediateQuery=priorRun/'immediate-query.bin';need(sha(immediateQuery)==route['routing']['immediate']['query_sha256'],'accepted immediate query')
 immediate_log=execute(immediate['path'],[STUDIO,immediateQuery,run/'immediate.bin'],'immediate-reused');need(sha(run/'immediate.bin')==route['routing']['immediate']['output_sha256'],'all1796 earlier immediate routing results preserved')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pre['protected'].items()) and all(sha(p)==h for p,h in inputs.items()),'all source/protected/original closure unchanged')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor313 dependencies unchanged')
 limits=['Owned header selects queued prebind visit order only; it does not retain/material-lookup or execute proxies, build GPU packets, read DMX/MDL/VTX files or supply a clock','Typed inputs require authenticated body/skin/LOD metadata, live native-width material IDs and known HasProxy observations; IDs are not dereferenced by this component','Normal native fixture has two single-model bodyparts, two skin families/LODs, duplicates and aliased material pointers; general bodygroup bases/multiple models and invalid input/allocation behavior have owned analytical/safety tests, not new native coverage','Deduplication by selected texture index matches queued helper; do not apply this to immediate per-mesh Bind, shared materials across models, or assume one proxy update per frame','Full IFM override/draw/replay, VEngineModel->VStudioRender026 bindings, renderable-data route, queued/immediate split and immediate body/mesh/context-Bind route are byte-checked static evidence. Earlier whole helper/immediate leaf executions remain boundary tests, not an executed combined draw chain','Queued prebind occurs on the submission side before queued rendering; whether later queue entries invoke proxy-related work is not generally proved. Full queue material snapshot lifetime remains unverified','Material clock source accessor reads signed tick/interval plus conditional offset, with different predicate branches. Actual SFM sample/shot/host writer selection remains UNVERIFIED; never replace it with frame/fps or slot50','Public proxy evaluation during owned session scopes remains refused; successful backup replay preserving out-of-override proxy writes requires a separate integration design/proof','Full editor, renderer and document/history unchanged; no original writes, live SFM/computer use, new dependency or publication']
 result=dict(status='VERIFIED owned x64 queued prebind selection; editor/clock composition INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',owned_safety=safety,safety_checks=int(m[1]),allocation_failures=int(m[2]),native_cases=1636,native_events=events,complete_trace_bytes=(run/'native.bin').stat().st_size,native_log=native_log,owned_log=owned_log,reused_immediate_log=immediate_log,reused_immediate_cases=1796,executables=exes,reused_immediate=immediate,source=sources,protected=pre['protected'],read_inputs=inputs,parent=pre['parent'],parent_sha256=pre['parent_sha256'],route_parent=pre['route'],editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,static_route_sha256=sha(run/'static-route.json'),trace_sha256=sha(run/'native.bin'),limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','safety_checks','allocation_failures','native_cases','native_events','complete_trace_bytes','reused_immediate_cases']},indent=2),flush=True)
if __name__=='__main__':main()
