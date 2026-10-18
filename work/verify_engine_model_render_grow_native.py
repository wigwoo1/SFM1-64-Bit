from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_engine_model_render_grow_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_model_render_grow_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_engine_model_render_grow.cpp /Fowork\\probe_model_render_grow_'+arch+'.obj /Fework\\probe_model_render_grow_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/model-render-grow-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_model_render_grow_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size==24960*32
b=subprocess.run(['work/probe_model_render_grow_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native record comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='grow_cases=24960'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll').read_bytes()).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
report={'cases':24960,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'engine_sha256':sha,'installation_unchanged':True,'limitations':'Original growth routine with observed allocator services only. 24960 states compare capacity, storage, allocator operation, bytes, old pointer, capacity committed before callback, call count and unchanged growth. Null/non-null old and returned pointers included; no original heap or full application lifetime validation. Signed overflow outside the bounded domain remains unverified.'}
Path('outputs/sfm64-engine-model-render-grow-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
