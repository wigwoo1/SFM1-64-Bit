from pathlib import Path
import subprocess,os,hashlib,json,re
records=[];baseline=None
for mode in (0,1):
 directory=Path(f'outputs/prefetch-pixel-mode-{mode}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 r=subprocess.run(['cmd.exe','/d','/c','work\\run_session_rate_grid_probe.cmd'],env=env,capture_output=True,text=True,timeout=60)
 log=r.stdout+r.stderr;Path(f'outputs/sfm64-prefetch-pixels-mode-{mode}.txt').write_text(log)
 assert r.returncode==0,log
 assert 'model_gpu_presentation_passed=1 frames=61' in log
 assert f'preparation_tasks={60 if mode else 0}' in log
 ticks=[tuple(map(int,x)) for x in re.findall(r'live_session_frame=(\d+) tick=(\d+)',log)]
 assert len(ticks)==61
 hashes=[]
 for frame in range(61):
  pixels=(directory/f'vulkan-retained-frame-{frame}.rgba').read_bytes()
  assert len(pixels)==512*512*4
  hashes.append(hashlib.sha256(pixels).hexdigest())
 for frame,reference in [(0,'mamagen-vulkan-sampled-shot-v1.rgba'),(60,'mamagen-vulkan-sampled-shot-second-tick-v1.rgba')]:
  assert (directory/f'vulkan-retained-frame-{frame}.rgba').read_bytes()==Path('outputs',reference).read_bytes()
 state=(ticks,hashes)
 if baseline is None:baseline=state
 else:assert state==baseline,'prefetch changed frame positions or source pixels'
 records.append(dict(prefetch=mode,frames=61,frame_sha256=hashes,ticks=ticks))
Path('outputs/sfm64-prefetch-pixel-equivalence.json').write_text(json.dumps(dict(all_61_frames_pixel_exact_between_modes=True,reference_endpoints_exact=True,runs=records,scope='GPU0, one model in one real saved session; native full parity unverified'),indent=2)+'\n')
print('61 frames and tick positions identical with prefetch off/on; immutable endpoint references exact')
