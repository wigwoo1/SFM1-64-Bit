from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_engine_model_render_assignment_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_model_render_assignment_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_engine_model_render_assignment.cpp /Fowork\\probe_model_render_assignment_'+arch+'.obj /Fework\\probe_model_render_assignment_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/model-render-assignment-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_model_render_assignment_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size==262144*8
b=subprocess.run(['work/probe_model_render_assignment_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native record comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='assignment_cases=262144'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll').read_bytes()).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
report={'cases':262144,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'engine_sha256':sha,'installation_unchanged':True,'limitations':'262144 original assignment states: all16bit flags, null/non-null supplied values, ffff/valid lowindices and highaliases. Exact flags/value, native unrelated bytes preserved. Fullcreation/service semantics/runtime ABI unverified.'}
Path('outputs/sfm64-engine-model-render-assignment-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
