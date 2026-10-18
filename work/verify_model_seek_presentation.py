from pathlib import Path
import re,json
log=Path('outputs/sfm64-model-seek-presentation-validation.txt').read_text();order=[int(n) for n in re.findall(r'Vulkan indexed model draw passed[^\n]* RGBA frame=(\d+)',log)]
assert order==[0,1,0,0,2],order
assert 'retained Vulkan sequence passed frames=5 available_frames=3' in log
assert 'material_uploads=1' in log and 'preparation_tasks=0' in log
assert 'model_gpu_presentation_passed=1 frames=5' in log
refs=('mamagen-vulkan-sampled-shot-v1.rgba','mamagen-vulkan-sampled-shot-second-tick-v1.rgba','mamagen-vulkan-sampled-shot-v1.rgba')
for n,name in enumerate(refs):assert Path(f'outputs/vulkan-retained-frame-{n}.rgba').read_bytes()==Path('outputs',name).read_bytes(),n
Path('outputs/sfm64-model-seek-presentation-summary.json').write_text(json.dumps({'frame_order':order,'resource_rebuilds':0,'material_uploads':1,'source_readbacks_exact':True,'speculative_prefetch':'disabled for caller-controlled selection','swapchain_visual_pixels':'unverified','keyboard_control':'not connected'},indent=2)+'\n')
print('controlled frame seeking and repeated redraws passed with source pixels exact')
