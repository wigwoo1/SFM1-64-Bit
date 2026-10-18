from pathlib import Path
import os,subprocess,json,re
records=[]
for label,count in [('derived',None),('conflict','2')]:
 directory=Path(f'outputs/auto-timeline-controls-{label}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')};env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/short-timeline-session.dmx',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_SESSION_TIMELINE='1',SFM64_DIAGNOSTIC_TIMELINE_BLANKS='1',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 if count:env['SFM64_DIAGNOSTIC_FRAME_COUNT']=count
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe','--control-test'],env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;Path(f'outputs/sfm64-auto-timeline-controls-{label}.txt').write_text(log,encoding='utf-8')
 if count:assert run.returncode==1 and 'playback frame count disagrees with saved timeline' in log and 'multi_model_scene_frame=' not in log
 else:
  assert run.returncode==0,log
  assert 'playback_count_source=saved_timeline count=3' in log
  assert [int(n) for n in re.findall(r'multi_model_scene_frame=(\d+) tick=',log)]==[0,2,0,1,2]
  for i in range(3):assert (directory/f'vulkan-retained-frame-{i}.rgba').read_bytes()==Path(f'outputs/auto-timeline-1-prefetch-0/vulkan-retained-frame-{i}.rgba').read_bytes()
 records.append(dict(test=label,outcome='rejected before draw' if count else 'boundary controls and pixels exact'))
env.pop('SFM64_DIAGNOSTIC_FRAME_COUNT',None);env.update(SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0')
run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe','--control-test'],env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;Path('outputs/sfm64-auto-timeline-controls-full-bounds.txt').write_text(log,encoding='utf-8');assert run.returncode==0,log
assert 'playback_count_source=saved_timeline count=20400' in log
assert [int(n) for n in re.findall(r'multi_model_scene_frame=(\d+) tick=',log)]==[0,20399,0,1,2]
records.append(dict(test='original_session_bounds',count=20400,selected_frames=[0,20399,0,1,2],scope='five blank samples, not full-session rendering'))
Path('outputs/sfm64-automatic-timeline-controls-gpu.json').write_text(json.dumps(records,indent=2),encoding='utf-8');print('automatic timeline controls passed no tick file/count, Home/End/step/play exact, conflicting count rejected, original 20400-frame bounds navigated')
