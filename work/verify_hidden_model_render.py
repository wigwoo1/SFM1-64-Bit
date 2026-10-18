from pathlib import Path
import subprocess,json
s=Path('work/run_session_rate_grid_probe.cmd').read_text().replace('roundtrip-11.dmx','hidden-model-session-v2.dmx').replace('FRAME_COUNT=61','FRAME_COUNT=3')
Path('work/run_hidden_model_probe.cmd').write_text(s)
r=subprocess.run(['cmd.exe','/d','/c','work\\run_hidden_model_probe.cmd'],capture_output=True,text=True,timeout=60)
log=r.stdout+r.stderr;Path('outputs/sfm64-hidden-model-render-validation.txt').write_text(log)
assert r.returncode==0,log
assert log.count('model_visible=0 changed_pixels=0')==3
for n in range(3):assert Path(f'outputs/vulkan-retained-frame-{n}.rgba').read_bytes()==bytes([13,13,20,255])*(512*512)
assert 'model_gpu_presentation_passed=1 frames=3' in log
Path('outputs/sfm64-hidden-model-render-summary.json').write_text(json.dumps({'frames':3,'hidden_model_draw_skipped':True,'all_source_pixels_equal_background':True,'native_inherited_visibility':'unverified'},indent=2)+'\n')
print('Hidden model verified: three presented frames, all source pixels equal background')
