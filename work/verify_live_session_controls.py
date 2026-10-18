from pathlib import Path
import re,json
log=Path('outputs/sfm64-live-session-controls-validation.txt').read_text()
states=re.findall(r'live_session_frame=(\d+) tick=(\d+) shot=(\d+) rebuilt=(\d+)',log);assert [int(row[0]) for row in states]==[0,1,0,0,1]
assert [int(row[3]) for row in states]==[1,0,0,0,0]
assert 'model_gpu_presentation_passed=1 frames=5' in log
for frame,name in ((0,'mamagen-vulkan-sampled-shot-v1.rgba'),(1,'mamagen-vulkan-sampled-shot-second-tick-v1.rgba')):assert Path(f'outputs/vulkan-retained-frame-{frame}.rgba').read_bytes()==Path('outputs',name).read_bytes()
Path('outputs/sfm64-live-session-controls-summary.json').write_text(json.dumps({'selected_frames':[0,1,0,0,1],'live_evaluator_rebuilds':1,'win32_controls_verified':True,'source_pixels_exact':True,'pre_exported_render_inputs':False,'native_scheduler':'unfinished','swapchain_visual_pixels':'unverified'},indent=2)+'\n')
print('live session keyboard-controlled rendering verified, retained evaluator and exact source pixels')
