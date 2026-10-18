from pathlib import Path
import os,subprocess,json,re
sequence=Path('work/reload-playback-rate-sessions.txt');sequence.write_text('work/roundtrip-11.dmx\nwork/movie-640x480-24fps-session.dmx\nwork/roundtrip-11.dmx\n')
records=[]
for label,override in [('saved',None),('explicit-30',655360030)]:
 directory=Path(f'outputs/reload-playback-rate-{label}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS='work/movie-target-reload-ticks.txt',SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE=str(sequence),SFM64_DIAGNOSTIC_SCENE_OUTPUT='movie',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_COUNT='3',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 if override:env['SFM64_DIAGNOSTIC_LIVE_RATE_PACKED']=str(override)
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe','--control-test'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-reload-playback-rate-{label}.txt').write_text(log)
 assert run.returncode==0,log
 rates=[int(v) for v in re.findall(r'playback_scene_rate_applied=(\d+)',log)]
 assert rates==([] if override else [655360024,655360060,655360024]),'presented saved rates not applied correctly'
 if override:assert 'playback_rate_source=explicit period_numerator_ns=1000000000 period_denominator=30' in log
 for n in (0,1):assert (directory/f'vulkan-retained-frame-{n}.rgba').read_bytes()==Path(f'outputs/movie-target-reload-sync/vulkan-retained-frame-{n}.rgba').read_bytes()
 records.append(dict(name=label,applied_saved_rates=rates,explicit_override_preserved=bool(override),control_pixels_exact=True))
Path('outputs/sfm64-reload-playback-rate-gpu.json').write_text(json.dumps(dict(saved_rate_changes_propagated=True,repeated_rate_no_reset=True,runs=records,scope='GPU0 hidden control test; cadence/anchor behavior tested separately, user-observed timing and native settings parity unverified'),indent=2)+'\n')
print('reload playback rate GPU controls passed: saved 24/60/24 applied on selected frames, repeated rate unchanged, explicit 30 preserved, pixels exact')
