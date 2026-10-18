from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_studio_decal_create_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_studio_decal_create_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_studio_decal_create.cpp /Fowork\\probe_studio_decal_create_'+arch+'.obj /Fework\\probe_studio_decal_create_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/studio-decal-create-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_studio_decal_create_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size==sum(2*(32+4+n*32) for n in range(1,129))
b=subprocess.run(['work/probe_studio_decal_create_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native record comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='create_cases=256'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll').read_bytes()).hexdigest();assert sha==json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text())['sha256']
report={'cases':256,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'studio_sha256':sha,'installation_unchanged':True,'limitations':'256 original full constructor states/counts1..128/stable and callback-halved descriptor counts. Exact array header/all entry bytes, returned identity, stored count, service counts/byte request and lock owner/depth compared. Node allocation/list insertion/heap/thread services observed; actual contention, allocation failure/overflow, real ownership and live renderer unverified.'}
Path('outputs/sfm64-studio-decal-create-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
