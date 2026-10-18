from pathlib import Path
import subprocess,os,json,hashlib,re
original='work/roundtrip-11.dmx';records=[]
ticks=Path('work/empty-scene-ticks.txt');ticks.write_text('870833\n870833\n870833\n')
clear=bytes([13,13,20,255])*(512*512)
reference=Path('outputs/material-overrides-prefetch-0/vulkan-retained-frame-0.rgba').read_bytes()
for label,session,model_count in [('empty','work/empty-scene-session.dmx',0),('hidden','work/hidden-model-scene-session.dmx',1)]:
 for mode in (0,1):
  sequence=Path(f'work/{label}-scene-session-sequence.txt');sequence.write_text('\n'.join([session,original,session])+'\n')
  directory=Path(f'outputs/{label}-scene-prefetch-{mode}');directory.mkdir(exist_ok=True)
  env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
  env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=session,SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE=str(sequence),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
  run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
  log=run.stdout+run.stderr;Path(f'outputs/sfm64-{label}-scene-prefetch-{mode}.txt').write_text(log)
  assert run.returncode==0,log
  assert [int(n) for n in re.findall(r'multi_model_scene_frame=\d+ tick=\d+ models=(\d+)',log)]==[model_count,1,model_count]
  pixels=[(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes() for n in range(3)]
  assert pixels[0]==clear and pixels[2]==clear,'empty frame retained stale geometry or incorrect clear'
  assert pixels[1]==reference,'visible scene after initial empty changed established pixels'
  assert 'geometry_allocations=2 material_uploads=1' in log
  assert 'retained_gpu_material_cache_entries=1' in log
  records.append(dict(scene=label,prefetch=mode,models=[model_count,1,model_count],clear_pixel_exact=True,visible_frame_matches_reference=True,frame_sha256=[hashlib.sha256(p).hexdigest() for p in pixels]))
Path('outputs/sfm64-empty-scene-gpu.json').write_text(json.dumps(dict(initial_empty_supported=True,initial_all_hidden_supported=True,return_to_empty_has_no_stale_geometry=True,runs=records,scope='GPU0 controlled saved scene and visibility fixtures with existing camera; not no-camera shots, native visibility semantics or full editor compatibility'),indent=2)+'\n')
print('empty and all-hidden initial scenes passed: clear -> visible -> clear, established pixels exact, both prefetch modes')
