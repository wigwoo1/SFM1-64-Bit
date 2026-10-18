from pathlib import Path
import subprocess,os,json,hashlib
ticks=Path('work/short-timeline-ticks.txt');ticks.write_text('870833\n870999\n871166\n');runs=[];reference=None
for mode in (0,1):
 for automatic in (False,True):
  directory=Path(f'outputs/auto-timeline-{int(automatic)}-prefetch-{mode}');directory.mkdir(exist_ok=True)
  env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')};env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/short-timeline-session.dmx',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_TIMELINE_BLANKS='1',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
  if automatic:env['SFM64_DIAGNOSTIC_SESSION_TIMELINE']='1'
  else:env['SFM64_DIAGNOSTIC_SCENE_TICKS']=str(ticks)
  run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;Path(f'outputs/sfm64-auto-timeline-{int(automatic)}-prefetch-{mode}.txt').write_text(log,encoding='utf-8');assert run.returncode==0,log
  pixels=[(directory/f'vulkan-retained-frame-{i}.rgba').read_bytes() for i in range(3)]
  if reference is None:reference=pixels
  assert pixels==reference,'automatic timeline pixels differ'
  if automatic:assert 'scene_frame_source=saved_timeline count=3 tick_array_bytes=0' in log
  runs.append(dict(automatic=automatic,prefetch=mode,sha256=[hashlib.sha256(p).hexdigest() for p in pixels]))
Path('outputs/sfm64-auto-timeline-gpu.json').write_text(json.dumps(runs,indent=2),encoding='utf-8');print('automatic saved timeline GPU passed 3 frames without tick file, explicit sequence pixels exact, both prefetch modes')
