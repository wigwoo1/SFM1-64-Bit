from pathlib import Path
import json
log=Path('outputs/sfm64-model-swapchain-device0-validation.txt').read_text();assert 'device=0 graphics_present_family=0' in log and 'device=1 graphics_present_family' not in log;assert 'model_gpu_presentation_passed=1 frames=3' in log
for frame,reference in enumerate(('mamagen-vulkan-sampled-shot-v1.rgba','mamagen-vulkan-sampled-shot-second-tick-v1.rgba','mamagen-vulkan-sampled-shot-v1.rgba')):assert Path(f'outputs/vulkan-retained-frame-{frame}.rgba').read_bytes()==Path('outputs',reference).read_bytes(),frame
Path('outputs/sfm64-model-presentation-device0-summary.json').write_text(json.dumps({'device':0,'presented_model_frames':3,'source_readbacks_byte_exact':True,'swapchain_visual_pixels':'unverified','cross_device_pixel_parity':'not achieved'},indent=2)+'\n')
print('original GPU: all 3 rendered source frames byte-exact while presentation enabled')
