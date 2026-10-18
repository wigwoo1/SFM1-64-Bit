from pathlib import Path
import os,subprocess,json,re,hashlib
records=[]
for name,controls in [('scripted',False),('win32-controls',True)]:
 directory=Path(f'outputs/session-seek-reload-{name}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS='work/movie-target-reload-ticks.txt',SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE='work/movie-target-reload-sessions.txt',SFM64_DIAGNOSTIC_SCENE_OUTPUT='movie',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_COUNT='3',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 args=['build/session-core/probe_vulkan_win32_surface_x64.exe']
 if controls:args.append('--control-test')
 else:env['SFM64_DIAGNOSTIC_SEEK_TEST']='1'
 run=subprocess.run(args,env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-session-seek-reload-{name}.txt').write_text(log)
 assert run.returncode==0,log
 frames=[int(n) for n in re.findall(r'multi_model_scene_frame=(\d+) tick=',log)]
 expected=[0,1,0,0,1 if controls else 2];assert frames==expected
 extents=[(int(w),int(h)) for w,h in re.findall(r'output=(\d+)x(\d+) RGBA frame=',log)]
 assert extents==[(640,480) if n==1 else (3840,2160) for n in expected],'seek retained wrong session dimensions'
 assert 'preparation_tasks=0' in log and 'preparation_worker_threads=0' in log
 hashes=[]
 for frame in sorted(set(expected)):
  pixels=(directory/f'vulkan-retained-frame-{frame}.rgba').read_bytes()
  reference=Path(f'outputs/movie-target-reload-sync/vulkan-retained-frame-{1 if frame==1 else 0}.rgba').read_bytes()
  assert pixels==reference,'seek returned stale session pixels'
  hashes.append(dict(frame=frame,sha256=hashlib.sha256(pixels).hexdigest()))
 records.append(dict(name=name,frame_selection=frames,output_extents=extents,stored_frame_sha256=hashes))
Path('outputs/sfm64-session-seek-reload-gpu.json').write_text(json.dumps(dict(backward_seek_reloads_actual_session=True,repeated_frame_zero_pixel_exact=True,scripted_and_win32_controls_passed=True,runs=records,scope='GPU0 scripted selection and hidden Win32 key/timer control test; not user-observed editor UI or complete interactive timing fidelity'),indent=2)+'\n')
print('session seek/reload passed: backward and repeated frame zero restore correct 4K session; scripted and Win32 controls match reference pixels')
