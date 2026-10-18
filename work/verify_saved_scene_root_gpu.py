from pathlib import Path
import os,subprocess,json,hashlib
records=[];base='work/roundtrip-11.dmx';relocated='work/relocated-root-session.dmx'
same=Path('work/root-reload-ticks.txt');same.write_text('870833\n870833\n870833\n')
sequence=Path('work/root-reload-sessions.txt');sequence.write_text('\n'.join([base,relocated,base])+'\n')
for name,session,root,reload in [('relocated-auto',relocated,None,False),('relocated-explicit',relocated,'100',False),('reload-auto',base,None,True),('invalid-root',base,'garbage',False),('invalid-reference',base,'4294967295',False)]:
 directory=Path(f'outputs/saved-scene-root-{name}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=session,SFM64_DIAGNOSTIC_SCENE_TICKS=str(same) if reload else 'work/topology-transition-sync-ticks.txt',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 if root is not None:env['SFM64_DIAGNOSTIC_SCENE_ROOT']=root
 if reload:env['SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE']=str(sequence)
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-saved-scene-root-{name}.txt').write_text(log)
 if name.startswith('invalid'):
  assert run.returncode==1,log
  assert ('scene root invalid' if name=='invalid-root' else 'session render root must reference film clip') in log
  records.append(dict(name=name,rejected=True));continue
 assert run.returncode==0,log
 assert f'scene_root_source={"explicit" if root else "activeClip"} root={2237 if reload else 100}' in log
 hashes=[]
 for n in range(3):
  pixels=(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes()
  reference=Path(f'outputs/scene-topology-transition-sync/vulkan-retained-frame-{0 if reload else n}.rgba').read_bytes()
  assert pixels==reference,'saved root selection changed established pixels'
  hashes.append(hashlib.sha256(pixels).hexdigest())
 records.append(dict(name=name,root=2237 if reload else 100,frame_sha256=hashes,pixel_exact=True))
Path('outputs/sfm64-saved-scene-root-gpu.json').write_text(json.dumps(dict(fixed_fixture_root_removed=True,relocated_root_pixels_exact=True,reload_reselects_saved_root=True,explicit_root_validated=True,runs=records,scope='GPU0 relocated equivalent saved session and original; activeClip selection native UI semantics unverified'),indent=2)+'\n')
print('saved scene roots passed: relocated root=100, explicit override, automatic reselection on reload, exact pixels, invalid roots rejected')
