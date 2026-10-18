from pathlib import Path
import os,subprocess,json,re
records=[]
for name,count in [('automatic',None),('conflict','2')]:
 directory=Path(f'outputs/scene-auto-count-{name}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS='work/movie-target-reload-ticks.txt',SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE='work/reload-playback-rate-sessions.txt',SFM64_DIAGNOSTIC_SCENE_OUTPUT='movie',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 if count:env['SFM64_DIAGNOSTIC_FRAME_COUNT']=count
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe','--control-test'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-scene-auto-count-{name}.txt').write_text(log)
 if count:
  assert run.returncode==1 and 'playback frame count disagrees with scene ticks' in log
  assert 'multi_model_scene_frame=' not in log
 else:
  assert run.returncode==0,log
  assert 'playback_count_source=scene_ticks count=3' in log
  assert [int(n) for n in re.findall(r'multi_model_scene_frame=(\d+) tick=',log)]==[0,2,0,1,2]
  for n in (0,1,2):assert (directory/f'vulkan-retained-frame-{n}.rgba').read_bytes()==Path(f'outputs/movie-target-reload-sync/vulkan-retained-frame-{1 if n==1 else 0}.rgba').read_bytes()
 records.append(dict(name=name,explicit_count=count,result='rejected conflict' if count else 'controls and reference pixels exact'))
Path('outputs/sfm64-scene-auto-count-gpu.json').write_text(json.dumps(dict(automatic_navigation_count=True,conflict_rejected_before_scene_draw=True,runs=records,scope='GPU0 hidden boundary-control test; no completed timeline editor claim'),indent=2)+'\n')
print('automatic scene frame count passed: no count option needed, boundary pixels exact, conflicting count rejected')
