from pathlib import Path
import os,subprocess,json
records=[]
for label,rate in [('saved',None),('explicit-24',655360024)]:
 directory=Path(f'outputs/scene-playback-rate-{label}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS='work/movie-target-reload-ticks.txt',SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE='work/movie-target-reload-sessions.txt',SFM64_DIAGNOSTIC_SCENE_OUTPUT='movie',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_COUNT='3',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 if rate is not None:env['SFM64_DIAGNOSTIC_LIVE_RATE_PACKED']=str(rate)
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe','--control-test'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-scene-playback-rate-{label}.txt').write_text(log)
 assert run.returncode==0,log
 fps=24 if rate else 60
 assert f'playback_rate_source={"explicit" if rate else "session"} period_numerator_ns=1000000000 period_denominator={fps}' in log
 for frame in (0,1):
  assert (directory/f'vulkan-retained-frame-{frame}.rgba').read_bytes()==Path(f'outputs/movie-target-reload-sync/vulkan-retained-frame-{frame}.rgba').read_bytes()
 records.append(dict(source='explicit' if rate else 'session',fps=fps,control_pixels_exact=True))
Path('outputs/sfm64-scene-playback-rate-gpu.json').write_text(json.dumps(dict(initial_scene_saved_rate_used=True,explicit_override_precedence_verified=True,runs=records,scope='hidden Win32 control path and configured clock; elapsed cadence verified separately, user-observed timing and rate changes during reload unverified'),indent=2)+'\n')
print('scene playback clock configuration passed: saved 60fps, explicit 24fps override, control pixels unchanged')
