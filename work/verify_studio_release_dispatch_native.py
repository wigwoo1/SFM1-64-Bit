from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_studio_release_dispatch_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_studio_release_dispatch_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_studio_release_dispatch.cpp /Fowork\\probe_studio_release_flow_'+arch+'.obj /Fework\\probe_studio_release_flow_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/studio-release-dispatch-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_studio_release_flow_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size>0
b=subprocess.run(['work/probe_studio_release_flow_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native record comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='release_dispatch_native_cases=8192'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll').read_bytes()).hexdigest();assert sha==json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text())['sha256']
report={'cases':8192,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'studio_sha256':sha,'installation_unchanged':True,'limitations':'Original126-byte Studio slot34 executed with relocatedglobals/cleanup/enqueue.8192 arbitraryowner/resource combinations acrossqueuepresent/configenabled gates match contextcallbackorder/direct-versusqueuedbranch/copiedargument/methodidentity. Nonnullcontextfixture; context/cleanup/queue observers, realruntime ownership/concurrency/GPU unverified.'}
Path('outputs/sfm64-studio-release-dispatch-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
