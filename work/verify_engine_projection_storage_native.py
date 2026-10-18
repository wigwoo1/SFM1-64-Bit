from pathlib import Path
import subprocess,json,hashlib,uuid
subprocess.run([r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe','work/prepare_engine_projection_storage_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_projection_storage_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_engine_projection_storage.cpp /Fowork\\probe_projection_storage_'+arch+'.obj /Fework\\probe_projection_storage_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/projection-storage-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_projection_storage_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size==65536*416
b=subprocess.run(['work/probe_projection_storage_64.exe',str(oracle)],check=True,capture_output=True,text=True)
assert 'native_matches=65536' in b.stdout
engine=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
sha=hashlib.sha256(engine.read_bytes()).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
report={'cases':65536,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'engine_sha256':sha,'installation_unchanged':True,'limitations':'Isolated copied original storage and cached-mask routines only, with eight constant and two call relocations; full plane generation, floating-point environment variations and live rendering remain unverified.'}
Path('outputs/sfm64-engine-projection-storage-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
