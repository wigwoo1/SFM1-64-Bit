from pathlib import Path
import os,subprocess,json,hashlib,re
original='work/roundtrip-11.dmx';small='work/movie-640x480-session.dmx'
ticks=Path('work/movie-target-reload-ticks.txt');ticks.write_text('870833\n870833\n870833\n')
sequence=Path('work/movie-target-reload-sessions.txt');sequence.write_text('\n'.join([original,small,original])+'\n')
records=[];baseline=None;small_pixels=None
for name,mode,independent in [('sync',0,False),('prefetch',1,False),('small-independent',0,True)]:
 directory=Path(f'outputs/movie-target-reload-{name}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=small if independent else original,SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),SFM64_DIAGNOSTIC_SCENE_OUTPUT='movie',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 if not independent:env['SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE']=str(sequence)
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-movie-target-reload-{name}.txt').write_text(log)
 assert run.returncode==0,log
 expected=[(640,480)]*3 if independent else [(3840,2160),(640,480),(3840,2160)]
 assert [(int(w),int(h)) for w,h in re.findall(r'output=(\d+)x(\d+) RGBA frame=',log)]==expected
 assert f'render_targets={1 if independent else 3}' in log
 assert 'retained_gpu_material_cache_entries=1' in log
 pixels=[(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes() for n in range(3)]
 assert [len(p) for p in pixels]==[w*h*4 for w,h in expected]
 hashes=[hashlib.sha256(p).hexdigest() for p in pixels]
 if independent:assert all(p==small_pixels for p in pixels),'resized output differs from independent target'
 else:
  assert pixels[0]==pixels[2]
  assert pixels[0]==Path('outputs/saved-movie-output-prefetch-0/vulkan-retained-frame-0.rgba').read_bytes()
  assert 'scene_target_rebuilt=640x480' in log and 'scene_target_rebuilt=3840x2160' in log
  if baseline is None:baseline=hashes;small_pixels=pixels[1]
  else:assert hashes==baseline,'prefetch changed resize output'
 fits=[(int(w),int(h)) for w,h in re.findall(r'viewport_fit x=\d+ y=\d+ width=(\d+) height=(\d+)',log)]
 assert len(fits)==3
 for (w,h),(source_w,source_h) in zip(fits,expected):assert abs(w/h-source_w/source_h)<.02
 records.append(dict(name=name,extents=expected,readback_bytes=[len(p) for p in pixels],frame_sha256=hashes,render_targets=1 if independent else 3))
Path('outputs/sfm64-movie-target-reload-gpu.json').write_text(json.dumps(dict(resolution_reload_supported=True,prefetch_pixel_exact=True,restored_4k_pixel_exact=True,small_frame_matches_independent_target=True,presentation_aspects_match=True,runs=records,scope='GPU0 controlled saved movie settings reload, normal successful allocation; no allocation-failure or native export parity claim'),indent=2)+'\n')
print('movie target reload passed: 4K -> 640x480 -> 4K, independent small target and restored 4K pixels exact, prefetch exact, presenter aspects correct')
