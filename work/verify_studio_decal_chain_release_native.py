from pathlib import Path
import subprocess,json,hashlib,uuid
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/prepare_studio_decal_release_native.py'],check=True)
for arch in ['32','64']:
 script=Path('work/build_studio_decal_chain_release_native_'+arch+'.cmd')
 script.write_text('@echo off\ncall "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars'+arch+'.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict '+('/arch:SSE2 ' if arch=='32' else '')+'work\\probe_studio_decal_chain_release.cpp /Fowork\\probe_studio_decal_chain_release_'+arch+'.obj /Fework\\probe_studio_decal_chain_release_'+arch+'.exe\nexit /b %errorlevel%\n')
 subprocess.run(['cmd','/c',str(script)],check=True)
oracle=Path('outputs/studio-decal-chain-release-'+uuid.uuid4().hex+'.bin')
a=subprocess.run(['work/probe_studio_decal_chain_release_32.exe',str(oracle)],check=True,capture_output=True,text=True)
assert oracle.stat().st_size>0
b=subprocess.run(['work/probe_studio_decal_chain_release_64.exe',str(oracle)],capture_output=True,text=True)
if b.returncode:print(b.stderr);raise RuntimeError('x64 native record comparison failed; oracle '+str(oracle))
assert b.stdout.strip()=='chain_native_cases=512'
sha=hashlib.sha256(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll').read_bytes()).hexdigest();assert sha==json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text())['sha256']
report={'cases':512,'native':a.stdout.strip(),'x64':b.stdout.strip(),'oracle':str(oracle),'studio_sha256':sha,'installation_unchanged':True,'limitations':'512 original full cleanup states, chain lengths1..64 and eight buffer ownership combinations. Exact nested field results, counter wrapping, chain free head, outer list/lock state and full service/free event order compared. Original569-byte caller plus137-byte innerunlink executed; actual unlink/count/head/tail/selfprevious compared. Association/list-remove/allocator/classifier observed. Actual list helper internals, heap ownership, relocation/contention and live renderer unverified.'}
Path('outputs/sfm64-studio-decal-chain-release-native-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
