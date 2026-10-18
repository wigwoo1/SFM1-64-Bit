from pathlib import Path
import subprocess,os,json,hashlib
base='work/roundtrip-11.dmx';changed='work/texture-transition-session.dmx'
ticks=Path('work/texture-transition-ticks.txt');ticks.write_text('870833\n870833\n870833\n')
sequences=Path('work/texture-transition-session-sequence.txt');sequences.write_text('\n'.join([base,changed,base])+'\n')
records=[];baseline=None;changed_pixels=None
for name,mode,session,sequence in [('sync',0,base,True),('prefetch',1,base,True),('changed-independent',0,changed,False)]:
 directory=Path(f'outputs/scene-texture-transition-{name}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=session,SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 if sequence:env['SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE']=str(sequences)
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-scene-texture-transition-{name}.txt').write_text(log)
 assert run.returncode==0,log
 assert f'retained_gpu_material_cache_entries={2 if sequence else 1}' in log
 assert f'material_uploads={2 if sequence else 1} render_targets=1' in log
 pixels=[(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes() for n in range(3)]
 assert all(len(p)==512*512*4 for p in pixels)
 if sequence:
  assert pixels[0]==pixels[2] and pixels[0]!=pixels[1],'texture change/restore pixel coverage'
  if baseline is None:baseline=pixels;changed_pixels=pixels[1]
  else:assert pixels==baseline,'prefetch changed resource transition output'
 else:assert all(p==changed_pixels for p in pixels),'new texture upload did not match independently rendered changed session'
 records.append(dict(name=name,frame_sha256=[hashlib.sha256(p).hexdigest() for p in pixels],gpu_cache_entries=2 if sequence else 1))
Path('outputs/sfm64-scene-texture-transition-gpu.json').write_text(json.dumps(dict(texture_change_verified=True,restore_pixel_exact=True,changed_frame_matches_independent_session=True,prefetch_pixel_exact=True,runs=records,scope='GPU0 controlled saved-session reload with fixed geometry and material addresses; not native rendering parity or full editor session replacement'),indent=2)+'\n')
print('GPU texture transition passed: changed pixels match independent session, restored entry reused, prefetch exact')
