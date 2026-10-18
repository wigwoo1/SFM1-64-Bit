from pathlib import Path
import subprocess,json,uuid,hashlib
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_engine_receiver_link_native.py'],check=True)
for arch in ['32','64']:
 p=Path('work/build_receiver_link_native_'+arch+'.cmd');p.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 work\\probe_engine_receiver_link.cpp /Fowork\\probe_receiver_link_'+arch+'.obj /Fework\\probe_receiver_link_'+arch+'.exe\nexit /b %errorlevel%\n');subprocess.run(['cmd','/c',str(p)],check=True)
oracle=Path('outputs/receiver-link-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_receiver_link_32.exe',str(oracle)],check=True,capture_output=True,text=True);b=subprocess.run(['work/probe_receiver_link_64.exe',str(oracle)],check=True,capture_output=True,text=True)
assert a.stdout.strip()==b.stdout.strip()=='receiver_link_cases=6438';assert oracle.stat().st_size==6438*532
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll').read_bytes()).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
r={'cases':6438,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'installation_unchanged':True,'engine_sha256':sha,'limitations':'Copied original insert/unlink only; logical links and head/tail/count compared across insertion, movement and removal. Isolated sentinel initialized empty-list cases included. Full association/resource callbacks and native allocation unverified.'};Path('outputs/sfm64-engine-receiver-link-native-validation.json').write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))
