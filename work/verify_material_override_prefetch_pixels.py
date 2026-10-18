from pathlib import Path
import subprocess,os,hashlib,json,re
records=[];baseline=None
references={frame:Path('outputs',name).read_bytes() for frame,name in [(0,'mamagen-vulkan-sampled-shot-v1.rgba'),(60,'mamagen-vulkan-sampled-shot-second-tick-v1.rgba')]}
reference_hashes={frame:hashlib.sha256(data).hexdigest() for frame,data in references.items()}
for mode in (0,1):
 directory=Path(f'outputs/material-overrides-prefetch-{mode}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 run=subprocess.run(['cmd.exe','/d','/c','work\\run_session_rate_grid_probe.cmd'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-material-overrides-prefetch-{mode}.txt').write_text(log)
 assert run.returncode==0,log
 assert 'model_gpu_presentation_passed=1 frames=61' in log
 assert f'preparation_tasks={60 if mode else 0}' in log
 assert 'live_material_override_records=7' in log
 assert 'material_uploads=1' in log
 ticks=[tuple(map(int,x)) for x in re.findall(r'live_session_frame=(\d+) tick=(\d+)',log)]
 assert len(ticks)==61
 hashes=[];differences={}
 for frame in range(61):
  pixels=(directory/f'vulkan-retained-frame-{frame}.rgba').read_bytes();assert len(pixels)==512*512*4
  hashes.append(hashlib.sha256(pixels).hexdigest())
  if frame in references:differences[frame]=sum(pixels[n:n+4]!=references[frame][n:n+4] for n in range(0,len(pixels),4))
 state=(ticks,hashes)
 if baseline is None:baseline=state
 else:assert state==baseline,'prefetch changed evaluated material output'
 records.append(dict(prefetch=mode,frames=61,frame_sha256=hashes,ticks=ticks,changed_pixels_from_legacy_references=differences))
for frame,name in [(0,'mamagen-vulkan-sampled-shot-v1.rgba'),(60,'mamagen-vulkan-sampled-shot-second-tick-v1.rgba')]:
 assert hashlib.sha256(Path('outputs',name).read_bytes()).hexdigest()==reference_hashes[frame]
Path('outputs/sfm64-material-overrides-prefetch-pixel-equivalence.json').write_text(json.dumps(dict(all_frames_pixel_exact_between_modes=True,legacy_references_preserved=True,runs=records,scope='GPU0 real session, saved texture overrides and animated alpha; native appearance and isolated alpha pixel behavior unverified'),indent=2)+'\n')
print('61 override-aware frames pixel-exact between prefetch modes; legacy references preserved; endpoint changed pixels='+str(records[0]['changed_pixels_from_legacy_references']))
