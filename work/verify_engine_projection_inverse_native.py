from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_engine_projection_inverse_native.py'],check=True)
subprocess.run([runtime,'work/prepare_engine_projection_inverse_harness.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_projection_inverse_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_engine_projection_inverse.cpp /Fowork\\probe_projection_inverse_'+arch+'.obj /Fework\\probe_projection_inverse_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/projection-inverse-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_projection_inverse_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size==131072*136
b=subprocess.run(['work/probe_projection_inverse_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native inverse comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='native_matches=131072'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll').read_bytes()).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
report={'cases':131072,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'engine_sha256':sha,'installation_unchanged':True,'limitations':'Original inverse routine with owned cookie check. Finite matrices, zero/singular/threshold cases, separate and aliased output; exceptional values, FP environments and renderer integration unverified.'}
Path('outputs/sfm64-engine-projection-inverse-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
