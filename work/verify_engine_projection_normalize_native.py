from pathlib import Path
import subprocess,json,hashlib,uuid
subprocess.run([r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe','work/prepare_engine_projection_normalize_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_projection_normalize_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_engine_projection_normalize.cpp /Fowork\\probe_projection_normalize_'+arch+'.obj /Fework\\probe_projection_normalize_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/projection-normalize-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_projection_normalize_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size==65536*28
b=subprocess.run(['work/probe_projection_normalize_64.exe',str(oracle)],check=True,capture_output=True,text=True)
assert 'native_matches=65536' in b.stdout
engine=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
sha=hashlib.sha256(engine.read_bytes()).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
report={'cases':65536,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'engine_sha256':sha,'installation_unchanged':True,'limitations':'Isolated copied original normalization routine with two constant relocations; finite-vector cases and explicit tiny/zero patterns only. NaN payloads, environment variations and full plane generation/live rendering remain unverified.'}
Path('outputs/sfm64-engine-projection-normalize-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
