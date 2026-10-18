from pathlib import Path
import subprocess,os,json,hashlib
runs={}
for mode in ('live','one','zero'):
 directory=Path(f'outputs/isolated-material-alpha-{mode}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_MATERIAL_ALPHA_MODE=mode,SFM64_DIAGNOSTIC_FRAME_PREFETCH='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 run=subprocess.run(['cmd.exe','/d','/c','work\\run_session_rate_grid_probe.cmd'],env=env,capture_output=True,text=True,timeout=60)
 log=run.stdout+run.stderr;Path(f'outputs/sfm64-isolated-material-alpha-{mode}.txt').write_text(log)
 assert run.returncode==0,log
 assert f'material_alpha_mode={mode}' in log and 'model_gpu_presentation_passed=1 frames=61' in log and 'material_uploads=1' in log
 runs[mode]=directory
def pixels(mode,frame):
 data=(runs[mode]/f'vulkan-retained-frame-{frame}.rgba').read_bytes();assert len(data)==512*512*4;return data
assert pixels('live',0)==pixels('one',0),'alpha=1 endpoint mismatch'
for frame in range(43,61):assert pixels('live',frame)==pixels('zero',frame),'alpha=0 endpoint mismatch'
changes={}
for frame in (0,20,43,60):
 one=pixels('one',frame);zero=pixels('zero',frame);live=pixels('live',frame)
 changes[frame]=dict(one_vs_zero=sum(one[n:n+4]!=zero[n:n+4] for n in range(0,len(one),4)),live_vs_one=sum(live[n:n+4]!=one[n:n+4] for n in range(0,len(one),4)),live_vs_zero=sum(live[n:n+4]!=zero[n:n+4] for n in range(0,len(one),4)))
assert changes[20]['live_vs_one']>0 and changes[20]['live_vs_zero']>0,'intermediate alpha not visible'
Path('outputs/sfm64-isolated-material-alpha-pixels.json').write_text(json.dumps(dict(alpha_one_endpoint_exact=True,alpha_zero_last_18_frames_exact=True,changed_pixels=changes,scope='GPU0 candidate visor alpha and dynamic pipeline; native Source depth/blending parity unverified'),indent=2)+'\n')
print('isolated alpha pixel validation passed: live=one at frame 0, live=zero at frames 43..60, intermediate alpha visibly differs; '+str(changes))
