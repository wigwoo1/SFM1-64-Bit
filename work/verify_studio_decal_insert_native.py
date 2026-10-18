from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_studio_decal_insert_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_studio_decal_insert_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_studio_decal_insert.cpp /Fowork\\probe_studio_decal_insert_'+arch+'.obj /Fework\\probe_studio_decal_insert_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/studio-decal-insert-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_studio_decal_insert_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size==1496*140
b=subprocess.run(['work/probe_studio_decal_insert_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native record comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='insert_cases=1496'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll').read_bytes()).hexdigest();assert sha==json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text())['sha256']
report={'cases':1496,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'studio_sha256':sha,'installation_unchanged':True,'limitations':'1496 original pointerinsert/frontier states, lengths1..16/all moved nodes/distinct targets+append. Exact normalized pointer links/head/tail/count compared, native12-byte payload unchanged. One valid frontier block/current nodes only; signed32 boundary behavior, selftarget/fresh nodes and live renderer ownership unverified.'}
Path('outputs/sfm64-studio-decal-insert-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
