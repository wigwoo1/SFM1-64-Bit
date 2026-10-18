from pathlib import Path
import os,subprocess,json,hashlib
records=[]
for name,empty,invalid_base in [('transition',False,False),('transition-invalid-base',False,True),('empty',True,False)]:
 directory=Path(f'outputs/scene-no-bootstrap-{name}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/empty-scene-session.dmx' if empty else 'work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 if not empty:env['SFM64_DIAGNOSTIC_SCENE_TICKS']='work/topology-transition-sync-ticks.txt'
 if invalid_base:env['SFM64_DIAGNOSTIC_PRESENT_MODEL']='work/__nonexistent_diagnostic_model__'
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-scene-no-bootstrap-{name}.txt').write_text(log)
 assert run.returncode==0,log
 assert 'scene_bootstrap_model_loaded=0' in log
 assert 'scene_bootstrap_geometry_workers=0' in log
 pixels=[]
 if empty:
  pixels=[(directory/'mamagen-vulkan-blended-draw-v1.rgba').read_bytes()]
  assert pixels[0]==bytes([13,13,20,255])*(512*512)
  assert 'retained_gpu_material_cache_entries=0' in log
 else:
  for n in range(3):
   p=(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes()
   assert p==Path(f'outputs/scene-topology-transition-sync/vulkan-retained-frame-{n}.rgba').read_bytes()
   pixels.append(p)
 records.append(dict(name=name,present_model_option='invalid' if invalid_base else 'absent',bootstrap_model_loaded=False,frame_sha256=[hashlib.sha256(p).hexdigest() for p in pixels]))
Path('outputs/sfm64-scene-no-bootstrap-gpu.json').write_text(json.dumps(dict(scene_path_independent_of_diagnostic_model=True,transition_frames_pixel_exact=True,empty_scene_pixel_exact=True,runs=records,scope='GPU0 explicit content directory with declared loose mounts; session assets still required when bound'),indent=2)+'\n')
print('scene rendering without bootstrap model passed: absent/invalid model options, exact shot transitions, empty scene with zero GPU materials')
