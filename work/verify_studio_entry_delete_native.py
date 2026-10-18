from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_studio_entry_delete_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_studio_entry_delete_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_studio_entry_delete.cpp /Fowork\\probe_studio_entry_delete_'+arch+'.obj /Fework\\probe_studio_entry_delete_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/studio-entry-delete-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_studio_entry_delete_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size>0
b=subprocess.run(['work/probe_studio_entry_delete_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native record comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='entry_delete_native_cases=520'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll').read_bytes()).hexdigest();assert sha==json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text())['sha256']
report={'cases':520,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'studio_sha256':sha,'installation_unchanged':True,'limitations':'Original197-byte deleting destructor plus109-byte12-node list helper executed. 520 cases: savedcounts0..64, flags0..3, external/owned storage. Every array byte and all populated12-byte nodes/payloads, return allocation identity and complete buffer/allocation free order compared. Nested lengths1..5; allocator/free services observed, actual heap/fullcaller/runtime integration unverified.'}
Path('outputs/sfm64-studio-entry-delete-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
