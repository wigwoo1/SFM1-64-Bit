from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_material_buffer_resize_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_material_buffer_resize_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_material_buffer_resize.cpp /Fowork\\probe_material_buffer_resize_'+arch+'.obj /Fework\\probe_material_buffer_resize_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/material-buffer-resize-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_material_buffer_resize_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size>0
b=subprocess.run(['work/probe_material_buffer_resize_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native record comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='buffer_resize_native_cases=14280'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll').read_bytes()).hexdigest();assert sha==json.loads(Path('outputs/sfm64-material-call-queue-recovery.json').read_text())['materialsystem_sha256']
report={'cases':14280,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'materialsystem_sha256':sha,'installation_unchanged':True,'limitations':'Original207-byte buffer resize executed.14280 states:oldpages0..16/requestedpages0..34/unalignedtarget/dirty/commit-success-failure/cursor0-half-end. Exact40-byte metadata/guards, result, commit-decommit-warning-OOM counts/arguments match x64. Servicesobserved; extreme32-bit overflow, callbackmutations, realVMimport identity/concurrency/GPU unverified.'}
Path('outputs/sfm64-material-buffer-resize-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
