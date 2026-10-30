"""Allowlisted public source/build snapshot; original files are never written."""
import hashlib,json,re,shutil,struct,zipfile
from pathlib import Path
from datetime import datetime,timezone
from editor_runtime_package import shader_manifest,owned_shader_payload,write_editor_package
ROOT=Path(__file__).resolve().parent.parent
PUB=ROOT/'publication'; STAGE=PUB/'stage'; REPO='wigwoo1/SFM1-64-Bit'
EXT={'.cpp','.h','.hpp','.c','.py','.cmd','.ps1','.inc','.hlsl','.hlsli'}
RAW=re.compile(r'(?:Bytes\d*|static (?:const )?(?:unsigned char|uint8_t)[^\n]*?)\[\]\s*=\s*\{[0-9, ]{500}')
SECRET=[re.compile(p) for p in [r'gh[pousr]_[A-Za-z0-9]{30,}',r'github_pat_[A-Za-z0-9_]{30,}',r'sk-proj-[A-Za-z0-9_-]{25,}',r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----']]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def write(name,s):
 p=STAGE/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(s,encoding='utf-8',newline='\n')
def main():
 STAGE.mkdir(parents=True,exist_ok=True);selected={};excluded=[]
 for p in sorted((ROOT/'work').iterdir()):
  if not p.is_file() or p.suffix.lower() not in EXT:continue
  try:s=p.read_bytes().decode('utf-8')
  except UnicodeDecodeError:excluded.append((p.name,'non-UTF8; needs review'));continue
  if RAW.search(s):excluded.append((p.name,'embedded original instruction array'));continue
  if any(r.search(s) for r in SECRET):raise RuntimeError('Possible credential in '+p.name)
  selected[p.relative_to(ROOT).as_posix()]=p
 selected['work/editor_shader_manifest.txt']=ROOT/'work/editor_shader_manifest.txt'
 for p in ROOT.iterdir():
  if p.is_file() and p.suffix.lower() in {'.cmd','.ps1'}:selected[p.name]=p
 vendor=ROOT/'work/dependencies/vulkan-headers-6802bb4733b6/Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1'
 for folder in ['include','LICENSES']:
  for p in sorted((vendor/folder).rglob('*')):
   if p.is_file() and (folder=='LICENSES' or p.suffix.lower()=='.h'):selected[p.relative_to(ROOT).as_posix()]=p
 for name in ['LICENSE.md','UPSTREAM_COMMIT.txt']:
  p=vendor/name
  if p.exists():selected[p.relative_to(ROOT).as_posix()]=p
 for p in (ROOT/'work/dependencies/lzma-decoder').iterdir():
  if p.is_file() and p.suffix.lower() in {'.c','.h','.md','.txt'}:selected[p.relative_to(ROOT).as_posix()]=p
 for p in PUB.iterdir():
  if p.is_file() and p.suffix.lower() in {'.py','.md'}:selected['publication/'+p.name]=p
 for name,p in selected.items():
  dest=STAGE/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dest)
 write('README.md',r'''# SFM1 64-Bit â€” experimental reconstruction

An independent native Windows x64 application working toward compatibility with
Source Filmmaker sessions, assets and editing workflows. This is unfinished
research and prototype code. It is not an official Valve release, a recompiled
Valve SFM, or a fully compatible replacement.

## Current state

The prototype has a Win32 editor, Vulkan rendering experiments, session parsing
and editing components, and many isolated compatibility tests. Complete animation,
material, lighting, particle and workflow fidelity is still unverified. Isolated
passing tests do not establish full application compatibility.

Save and Save As intentionally save **new copies only**. Existing files must not
be replaced. Keep originals and backups; experimental output requires validation.

## Download and run

The current Windows x64 development build is in
[downloads/sfm64-experimental-windows-x64.zip](downloads/sfm64-experimental-windows-x64.zip).
Open that file on GitHub, use **Download raw file**, then extract the ZIP.
It includes usage instructions, its build manifest and SHA-256 checksums.
A fresh compile and AMD64/PE32+ header check are recorded. This publication run
did not repeat interactive rendering or establish visual equivalence.

You need your own installed SFM content and a working Vulkan driver. No Valve
assets, sessions or runtime DLLs are included. See `RUNNING.txt` in the ZIP.

## Build the complete editor

Use a short checkout path on Windows (long Vulkan header paths can exceed the
compiler path limit). Install Visual Studio C++ Build Tools, Windows SDK, and
the DirectX Shader Compiler (DXC). Set `SFM64_DXC_EXE` to DXC if it is not in
the development-machine location named by the build script, then run:

```bat
work\build_sfm64_editor_x64.cmd
```

The script names a particular local Visual Studio installation; adjust that path
for your installation. Licensed Vulkan headers and the LZMA decoder source subset
are included under `work/dependencies/`. The output is
`outputs/sfm64-editor-x64/sfm64_session_ui.exe`.
The build also compiles the explicitly listed project-authored Vulkan shaders
into a `shaders/` directory beside that executable. Keep that directory with
the executable when copying or extracting the build. The editor resolves it
from its own location, independent of the current working directory.
`build-session-core.cmd` builds the session core, not the complete editor.

Many historical tests require local SFM or fixture/evidence files and still
contain development-machine paths. They are research utilities, not a portable
CI suite. Original machine-code oracle arrays and proprietary fixtures are
excluded; tests that need them cannot run from this public snapshot alone.
The application does not depend on the x86 reference oracles.

## Publication contents and updates

Project-authored source/scripts and licensed source dependencies are included.
Dumps, logs, copied original binaries, game assets, sessions, `.blend` files,
object files and private workspace metadata stay local.
`publication/prepare_public_snapshot.py` defines the allowlist;
`docs/publication-manifest.json` records published file identities.

Daily synchronization is requested through the local Codex automation. It requires
the computer and Codex to be running with the workspace and GitHub connection
available. GitHub alone cannot upload unpublished files from this PC.

## Rights and third-party notices

Source Filmmaker and Source are Valve trademarks. This project is unaffiliated.
No blanket open-source license is granted in this snapshot. Third-party notices
remain with included Vulkan headers and LZMA decoder source. Public visibility
does not grant rights to redistribute Valve software or assets.
''')
 write('.gitignore','/build/\n/outputs/\n/publication/stage/\n/publication/payloads/\n/publication/state.json\n/publication/*.local.json\n*.obj\n*.pdb\n*.ilk\n*.log\n__pycache__/\n.env*\n')
 write('THIRD_PARTY_NOTICES.md','''# Third-party source notices

Vulkan-Headers: Khronos Group, pinned revision
6802bb4733b63ed5efd3adb308a6c885ef180ea1. Apache-2.0 OR MIT;
full texts and file notices remain in the included dependency directory.
Upstream: https://github.com/KhronosGroup/Vulkan-Headers

LZMA decoder subset: Igor Pavlov / 7-Zip, revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Public-domain notices remain
in source; see work/dependencies/lzma-decoder/README.md.
Upstream: https://github.com/ip7z/7zip

Valve binaries, assets, sessions, disassembly dumps and generated embedded
original instruction arrays are excluded from this publication.
''')
 build=ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe';raw=build.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
 if struct.unpack_from('<H',raw,pe+4)[0]!=0x8664 or struct.unpack_from('<H',raw,pe+24)[0]!=0x20b:raise RuntimeError('Editor must be AMD64 PE32+')
 log=(ROOT/'outputs/sfm64-editor-x64/build.log').read_text(errors='replace');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',log)
 graph=set()
 def visit(name):
  if name in graph:return
  graph.add(name);p=ROOT/name
  for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(errors='replace'),re.M):
   child=(p.parent/inc).relative_to(ROOT).as_posix()
   if (ROOT/child).exists():
    if child not in selected:raise RuntimeError('Editor include excluded '+child)
    visit(child)
 visit('work/probe_vulkan_win32_surface_x64.cpp')
 shader_rows=shader_manifest(ROOT)
 referenced={name for path in graph for name in re.findall(r'"(?:build/session-core/)?(sfm64-[a-z0-9-]+\.spv)"',(ROOT/path).read_text(errors='replace'))}
 if referenced!={stem+'.spv' for _,stem in shader_rows}:raise RuntimeError('Editor shader manifest does not cover renderer references')
 for name in ['work/editor_shader_manifest.txt','work/build_editor_shader_resources.cmd','work/build_sfm64_editor_x64.cmd']:visit(name)
 for _,stem in shader_rows:visit('work/'+stem+'.hlsl')
 shaders=owned_shader_payload(ROOT,build.parent)
 info=dict(target='AMD64 PE32+',source='work/probe_vulkan_win32_surface_x64.cpp',compiled_utc=datetime.fromtimestamp(build.stat().st_mtime,timezone.utc).isoformat(),executable_sha256=sha(build),editor_source_dependencies={k:sha(STAGE/k) for k in sorted(graph)},runtime_shader_sha256={k:hashlib.sha256(v).hexdigest() for k,v in shaders.items()},compiler_warnings=warnings,runtime_test_this_publication='NOT RUN',compatibility='INCOMPLETE')
 runtime=r'''SFM1 64-Bit experimental development build

Requires Windows x64, a Vulkan driver, and your own SFM installation.
No SFM content or Valve runtime DLLs are included.
Keep the included shaders/ directory beside sfm64_session_ui.exe. These are
compiled from project-authored HLSL sources, not copied Valve shader binaries.

In a command prompt in this extracted folder, replace both example paths:

sfm64_session_ui.exe --interactive --session "X:\path\session.dmx" --content-directory "X:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod" --start-frame 120

Keep originals backed up. Save/Save As requests a new filename and saves copies.
Full SFM compatibility and visual fidelity remain incomplete/unverified.
BUILD.json records compile time, architecture, warnings and source hashes.
This publication did not repeat interactive runtime/render testing.
'''.replace('\\"','"')
 package=STAGE/'downloads/sfm64-experimental-windows-x64.zip';package.parent.mkdir(parents=True,exist_ok=True)
 write_editor_package(package,raw,shaders,json.dumps(info,indent=2).encode(),runtime.encode(),(STAGE/'THIRD_PARTY_NOTICES.md').read_bytes())
 write('docs/build-status.json',json.dumps(info,indent=2)+'\n')
 write('docs/publication-policy.json',json.dumps(dict(repository=REPO,excluded_reference_sources=excluded,policy='Only allowlisted project source and licensed dependencies; original binaries/assets, sessions, dumps and private metadata stay local.'),indent=2)+'\n')
 write('docs/sync-status.json',json.dumps(dict(checked_utc=datetime.now(timezone.utc).isoformat(),status='PUBLICATION PREPARED; inspect commit for remote result',repository=REPO),indent=2)+'\n')
 extra=['README.md','.gitignore','THIRD_PARTY_NOTICES.md','docs/build-status.json','docs/publication-policy.json','docs/sync-status.json','downloads/sfm64-experimental-windows-x64.zip']
 paths=sorted(set(selected)|set(extra));manifest={k:dict(sha256=sha(STAGE/k),size=(STAGE/k).stat().st_size) for k in paths}
 write('docs/publication-manifest.json',json.dumps(dict(repository=REPO,files=manifest),indent=2)+'\n');paths.append('docs/publication-manifest.json')
 prev=json.loads((PUB/'state.json').read_text()) if (PUB/'state.json').exists() else {'files':{}}
 changes=[];large=[]
 for name in paths:
  p=STAGE/name;data=p.read_bytes()
  if prev.get('files',{}).get(name)==hashlib.sha256(data).hexdigest():continue
  if p.suffix=='.zip' or len(data)>150000:large.append(dict(path=name,size=len(data),sha256=sha(p)))
  else:changes.append(dict(path=name,mode='100644',type='blob',content=data.decode('utf-8')))
 dest=PUB/'payloads';dest.mkdir(exist_ok=True);batches=[];batch=[];size=0
 for e in changes:
  n=len(json.dumps(e))
  if batch and n+size>350000:batches.append(batch);batch=[];size=0
  batch.append(e);size+=n
 if batch:batches.append(batch)
 for i,b in enumerate(batches):(dest/('tree-'+str(i)+'.json')).write_text(json.dumps(b),encoding='utf-8')
 result=dict(repository=REPO,stage=str(STAGE),tree_batches=len(batches),large_blobs=large,files={name:sha(STAGE/name) for name in paths},source_files=len(selected),file_count=len(paths),excluded_sources=len(excluded),build_package_sha256=sha(package),compiler_warning_count=len(warnings))
 (PUB/'prepared.local.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:v for k,v in result.items() if k not in ['files','large_blobs']}));print('large blobs',len(large))
if __name__=='__main__':main()
