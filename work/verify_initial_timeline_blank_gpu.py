from pathlib import Path
import subprocess,os,json,hashlib,re
clear=bytes([13,13,20,255])*(512*512)
reference=Path('outputs/material-overrides-prefetch-0/vulkan-retained-frame-0.rgba').read_bytes()
runs=[]
for label,start in [('gap',625333),('scene-less',0)]:
 ticks=Path(f'work/initial-{label}-timeline-ticks.txt');ticks.write_text(f'{start}\n870833\n{start}\n')
 for mode in (0,1):
  directory=Path(f'outputs/initial-timeline-{label}-prefetch-{mode}');directory.mkdir(exist_ok=True)
  env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
  env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK=str(start),SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),SFM64_DIAGNOSTIC_TIMELINE_BLANKS='1',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
  run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;Path(f'outputs/sfm64-initial-timeline-{label}-prefetch-{mode}.txt').write_text(log,encoding='utf-8');assert run.returncode==0,log
  pixels=[(directory/f'vulkan-retained-frame-{i}.rgba').read_bytes() for i in range(3)]
  assert pixels[0]==clear and pixels[2]==clear,'initial/returned blank differs'
  assert pixels[1]==reference,'scene after initial blank differs'
  assert 'frozen_scene_models=0 vertices=0 indices=0' in log
  assert 'retained_gpu_material_cache_entries=1' in log
  runs.append(dict(initial_state=label,prefetch=mode,pixels_exact=True,sha256=[hashlib.sha256(p).hexdigest() for p in pixels]))
Path('outputs/sfm64-initial-timeline-blank-gpu.json').write_text(json.dumps(runs,indent=2),encoding='utf-8')
print('initial gap and scene-less frames GPU passed blank -> scene -> blank, no bootstrap model, both prefetch modes pixels exact')
