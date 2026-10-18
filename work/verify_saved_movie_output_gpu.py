from pathlib import Path
import subprocess,os,json,hashlib,re
records=[];baseline=None
for mode in (0,1):
 directory=Path(f'outputs/saved-movie-output-prefetch-{mode}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS='work/topology-transition-sync-ticks.txt',SFM64_DIAGNOSTIC_SCENE_OUTPUT='movie',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-saved-movie-output-prefetch-{mode}.txt').write_text(log)
 assert run.returncode==0,log
 assert 'scene_output_extent=3840x2160' in log
 assert len(re.findall(r'output=3840x2160 RGBA frame=',log))==3
 hashes=[]
 for n in range(3):
  pixels=(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes();assert len(pixels)==3840*2160*4
  assert pixels!=bytes([13,13,20,255])*(3840*2160),'movie frame contains no geometry'
  hashes.append(hashlib.sha256(pixels).hexdigest())
 assert hashes[0]==hashes[2] and hashes[0]!=hashes[1]
 if baseline is None:baseline=hashes
 else:assert hashes==baseline,'movie output changed between prefetch modes'
 fits=[(int(w),int(h)) for w,h in re.findall(r'viewport_fit x=\d+ y=\d+ width=(\d+) height=(\d+)',log)]
 assert len(fits)==3 and all(abs(w/h-16/9)<.02 for w,h in fits),'presenter did not fit movie aspect'
 records.append(dict(prefetch=mode,frames=3,width=3840,height=2160,frame_sha256=hashes,presentation_fits=fits))
Path('outputs/sfm64-saved-movie-output-gpu.json').write_text(json.dumps(dict(saved_movie_dimensions_used=True,readback_bytes_per_frame=33177600,prefetch_pixel_exact=True,backward_seek_pixel_exact=True,aspect_fit_verified=True,runs=records,scope='GPU0 4K candidate shader scene rendering and swapchain blit; native export/shading parity and resolution changes on reload unverified'),indent=2)+'\n')
print('saved 3840x2160 movie GPU output passed: camera/target/readback/presentation extent, shot transitions, prefetch and return pixels exact')
