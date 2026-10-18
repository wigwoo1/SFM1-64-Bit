from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_studio_queued_enqueue_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_studio_queued_enqueue_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_studio_queued_enqueue.cpp /Fowork\\probe_studio_queued_enqueue_'+arch+'.obj /Fework\\probe_studio_queued_enqueue_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/studio-queued-enqueue-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_studio_queued_enqueue_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size>0
b=subprocess.run(['work/probe_studio_queued_enqueue_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native record comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='queued_enqueue_cases=8192'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll').read_bytes()).hexdigest();assert sha==json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text())['sha256']
report={'cases':8192,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'studio_sha256':sha,'installation_unchanged':True,'limitations':'Original90-byte queue helper:8192 allocation success/failure states. Native28-byte request, vtable writes, refcount1/copiedmethod/owner/argument, untouchedpadding/guards and queuecallback(null onfailure) checked. x64 copiedfields/servicecounts match; syntheticallocator and queue slot0 observers, realqueue ownership/heap/concurrency/renderer unverified.'}
Path('outputs/sfm64-studio-queued-enqueue-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
