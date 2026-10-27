"""Fresh owned x64 model/material bridge tests. Full renderer is not admitted."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, re, struct, subprocess, tempfile, time
from verify_builtin_error_material_seed import source_closure
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/model-material-prebind-bridge-20261027'
GAME=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
SESSION=GAME/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
DUMP=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
NEW=['model_material_prebind_draw_candidate.h','test_model_material_prebind_draw.cpp','build_model_material_prebind_draw.cmd','verify_model_material_prebind_draw.py']
def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def need(b,s):
 if not b:raise RuntimeError(s)
def main():
 pre=json.loads((OUT/'preflight.json').read_text());parent=json.loads(Path(pre['parent']).read_text())
 need(sha(pre['parent'])==pre['parent_sha256']=='8363f65ce029b99977997d388e2ef2fdd9f0c432a0e2035743911a9e6430937c','accepted previous CPU draw transaction')
 pinned={**pre['protected'],**pre['additional_inputs']}
 need(all(sha(p)==h for p,h in pinned.items()),'1017 protected and five new original input identities before')
 sources=source_closure([Path(p) for p in parent['source']]+[ROOT/'work'/name for name in NEW])
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));print('Verification directory: '+str(run),flush=True)
 started=time.time();p=subprocess.run(['cmd.exe','/d','/c',str(ROOT/'work/build_model_material_prebind_draw.cmd')],cwd=ROOT,capture_output=True,text=True,timeout=180);(run/'build-driver.log').write_text(p.stdout+p.stderr);need(p.returncode==0,'fresh owned bridge build')
 exe=OUT/'model_prebind_owned_x64.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
 need(exe.stat().st_mtime>=started-2 and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b and b'asInvoker' in raw,'fresh AMD64 PE32+ asInvoker executable')
 log=(OUT/'bridge-build.log').read_text();(run/'bridge-build.log').write_text(log)
 warnings=re.findall(r'([^\r\n]+): warning (C\d+): ([^\r\n]+)',log)
 need(len(warnings)==1 and warnings[0][1]=='C4458' and 'graph_transform_hierarchy.h(16)' in warnings[0][0] and "'visiting' hides class member" in warnings[0][2] and not re.search(r'\berror C\d+',log),'only known unchanged hierarchy warning; no new warnings/errors')
 inspect=subprocess.run([str(DUMP),'/headers','/imports',str(exe)],capture_output=True,text=True,check=True).stdout;(run/'bridge-inspection.txt').write_text(inspect)
 modules=re.findall(r'^\s+([\w.\-]+\.dll)\s*$',inspect,re.I|re.M);need(modules==['KERNEL32.dll'],'owned executable system-only direct imports')
 def execute(path,args,label):
  p=subprocess.run([str(path)]+list(map(str,args)),cwd=run,capture_output=True,text=True,timeout=180);(run/(label+'.log')).write_text(p.stdout+p.stderr);need(p.returncode==0,label+' failed: '+p.stdout+p.stderr);return p.stdout.strip()
 safety=execute(exe,[],'owned-safety');m=re.search(r'checks=(\d+) actual_allocation_failures=(\d+) post_consumer_failures=(\d+)',safety)
 need(m and int(m[1])>=3691 and int(m[2])>=1822 and int(m[3])>0,'controlled selection/resource/refusal/clock/real allocation coverage')
 benchmark=execute(exe,['--benchmark',SESSION,GAME],'actual-benchmark')
 lines=[line.split('\t') for line in benchmark.splitlines()];model=next(v for v in lines if v[0]=='MODEL');materials=[v for v in lines if v[0]=='MATERIAL'];selected=[v for v in lines if v[0]=='SELECTED'];unsupported=[v[1] for v in lines if v[0]=='UNSUPPORTED']
 need(model[1:4]==['13817','1','0'] and model[4]==(GAME/'usermod/models/weapons/w_models/w_flamethrower.mdl').as_posix() and model[5:]==['2','3'],'actual mounted MDL/VVD/VTX metadata and session skin/body')
 want=[GAME/'tf/materials/models/weapons/c_items/c_flamethrower.vmt',GAME/'tf/materials/models/effects/pyro/pilotlight.vmt',GAME/'tf/materials/models/weapons/c_items/c_flamethrower_blue.vmt']
 need(materials==[['MATERIAL',str(i),path.as_posix(),status] for i,(path,status) in enumerate(zip(want,['REFUSED','INITIALIZED','REFUSED']))],'all actual filename materials and honest initialization results')
 need(selected==[['SELECTED','2','c_flamethrower_blue'],['SELECTED','1','models/effects/pyro/pilotlight']],'actual selected blue skin and pilot ordering')
 need(unsupported==['AnimatedWeaponSheen','invis','ModelGlowColor','YellowLevel','Multiply']*2,'complete unsupported factory names for both weapon materials')
 need('full_model=REFUSED_SAFE observer_calls=0 GPU=NOT_RUN' in benchmark,'complete selected model safely refuses; no successful partial render claim')
 # Reuse exact previously accepted programs; no dependency in them changed.
 regressions={};reused={}
 old=parent['executables']['draw'];need(sha(old['path'])==old['sha256']=='0cc0295ee97d78ec07cf08acb9069e82d0e662203c754c509ab8a9a724aedb92','exact accepted earlier draw executable');reused['draw']=old;regressions['draw']=execute(old['path'],[],'draw-reused');need('checks=2831 actual_allocation_failures=1388' in regressions['draw'],'all earlier composed transaction safety unchanged')
 selectorReceipt=ROOT/'outputs/studio-proxy-prebind-owned-20261027/verification-wi3kmc6t/results.json';need(sha(selectorReceipt)=='c20e68de47f58101d1a00bd25297d380926e46e59bf262d8f4e8d6f07d5ef623','accepted earlier selector receipt');selector=json.loads(selectorReceipt.read_text());old=selector['executables']['owned'];need(sha(old['path'])==old['sha256'],'exact accepted selector executable');reused['selector']=old;regressions['selector']=execute(old['path'],[],'selector-reused');need('checks=36 allocation_failures=6' in regressions['selector'],'earlier body/skin/LOD/alias safety unchanged')
 need(all(sha(p)==h for p,h in sources.items()) and all(sha(p)==h for p,h in pinned.items()),'complete current source/protected/original input closure unchanged')
 need(sha(pre['editor'])==pre['editor_sha256'] and all(sha(ROOT/p)==h for p,h in pre['editor_dependencies'].items()),'editor executable and all313 dependencies unchanged')
 limits=['Independent reconstruction remains INCOMPLETE; this is an opt-in CPU bridge, not editor or GPU renderer admission, complete Source rendering, or full native draw-chain execution',
 'New controlled geometry is authored typed metadata without triangles. Actual benchmark geometry is decoded read-only from mounted original MDL49/VVD/VTX7 LOD0; loader association assetKey to immutable geometry is caller-owned, not automatically authenticated by the bridge',
 'Only ordinary checked IFM puppet models, normal queued-prebind selection at LOD0, current session skin/body, initialized retained filename resources. No forced material or higher LOD scheduling implemented here',
 'Token checks store identity, current nonrecycled ID/name, live reference, initialized complete proxy state and filename cache category. Caller keeps store/resources alive on its owner thread; tokens acquire no references and do not support asynchronous use',
 'Real benchmark model13817 skin1 selects blue weapon and pilot. Both weapon VMTs refuse five unsupported proxy types. Complete selected model safely refuses before consumer; pilot state preserved. This is a verified compatibility gap, not a completed benchmark render',
 'Supported controlled test invokes actual Equals, Sine and weapon_invis implementations. Explicit time is frozen; missing required time refuses. Actual SFM shot/sample/frame to original clock mapping remains UNVERIFIED; no frame/fps or wall time substitution',
 'Snapshots/journals are borrowed CPU values for callback only; no texture/GPU retention. No-proxy visits get guarded snapshots but execute no invented native OnBind',
 'Owned parameter rollback includes allocation failure after consumer during final Graph validation. External consumer effects, explicit graph edits, loaders, third-party proxy side effects and GPU work are not globally rolled back',
 'One pre-existing C4458 hierarchy shadow warning remains; file protected unchanged. Prior draw and selection safety executables reused by exact accepted hash, not freshly compiled this turn',
 'No SFM/computer use, original writes, document/history/serialization/rendering changes, new dependencies or publication; full editor313-source closure unchanged so editor not rebuilt']
 result=dict(status='VERIFIED bounded owned x64 model/material bridge; actual benchmark full-model compatibility REFUSED / INCOMPLETE',utc=datetime.now(timezone.utc).isoformat(),previous_goal_turn='progress',safety=safety,safety_checks=int(m[1]),actual_allocation_failures=int(m[2]),post_consumer_failures=int(m[3]),benchmark=benchmark,selected_textures=selected,unsupported_proxies=unsupported,executables={'bridge':dict(path=str(exe),sha256=sha(exe),machine='0x8664',optional_header='0x20b',imports=modules,fresh=True)},compiler_warnings=warnings,reused_executables=reused,regressions=regressions,source=sources,protected=pre['protected'],read_inputs=pre['additional_inputs'],parent=pre['parent'],parent_sha256=pre['parent_sha256'],editor=pre['editor'],editor_sha256=pre['editor_sha256'],editor_dependencies=pre['editor_dependencies'],editor_rebuilt=False,limits=limits)
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(run/'results.json'),receipt_sha256=sha(run/'results.json')),indent=2)+'\n')
 print(json.dumps({k:result[k] for k in ['status','safety_checks','actual_allocation_failures','post_consumer_failures','selected_textures','unsupported_proxies','regressions']},indent=2),flush=True)
if __name__=='__main__':main()
