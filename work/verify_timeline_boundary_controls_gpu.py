from pathlib import Path
import subprocess,os,json,re
directory=Path('outputs/timeline-boundary-controls');directory.mkdir(exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS='work/movie-target-reload-ticks.txt',SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE='work/reload-playback-rate-sessions.txt',SFM64_DIAGNOSTIC_SCENE_OUTPUT='movie',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_COUNT='3',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe','--control-test'],env=env,capture_output=True,text=True,timeout=60)
log=run.stdout+run.stderr;Path('outputs/sfm64-timeline-boundary-controls-gpu.txt').write_text(log)
assert run.returncode==0,log
frames=[int(n) for n in re.findall(r'multi_model_scene_frame=(\d+) tick=',log)];assert frames==[0,2,0,1,2]
titles=re.findall(r'playback_window_title=(.+)',log)
assert titles==[f'SFM64 prototype - Frame {n+1} / 3 - {"Playing" if i==4 else "Paused"}' for i,n in enumerate(frames)]
assert [int(v) for v in re.findall(r'playback_scene_rate_applied=(\d+)',log)]==[655360024,655360060]
for n in (0,1,2):assert (directory/f'vulkan-retained-frame-{n}.rgba').read_bytes()==Path(f'outputs/movie-target-reload-sync/vulkan-retained-frame-{1 if n==1 else 0}.rgba').read_bytes()
Path('outputs/sfm64-timeline-boundary-controls-gpu.json').write_text(json.dumps(dict(home_end_navigation_verified=True,frame_selection=frames,titles=titles,reference_pixels_exact=True,rates_updated=True,scope='GPU0 hidden Win32 message/control test, not a complete timeline/editor UI or user-observed latency'),indent=2)+'\n')
print('timeline boundary controls passed: End/Home select final/first, frame/play titles match, session rates/resolutions/pixels restored')
