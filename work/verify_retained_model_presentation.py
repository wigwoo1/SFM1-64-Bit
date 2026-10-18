from pathlib import Path
import json
log=Path('outputs/sfm64-retained-model-presentation-validation.txt').read_text()
assert 'model_gpu_presentation_passed=1 frames=9' in log and 'presentation_resource_sets=1' in log
assert log.count('source=model_gpu_image frames=1')==9
assert log.count('present_semaphore=per_image per_frame_queue_idle=0')==12
assert log.count('present_result=0')==12
refs=('mamagen-vulkan-sampled-shot-v1.rgba','mamagen-vulkan-sampled-shot-second-tick-v1.rgba','mamagen-vulkan-sampled-shot-v1.rgba')
for frame in range(9):assert Path(f'outputs/vulkan-retained-frame-{frame}.rgba').read_bytes()==Path('outputs',refs[frame%3]).read_bytes(),frame
Path('outputs/sfm64-retained-model-presentation-summary.json').write_text(json.dumps({'device':0,'model_frames':9,'presenter_resource_sets':1,'source_readbacks_exact':True,'synchronization':'submission fence and per-image present semaphore; queue idle at teardown','swapchain_visual_pixels':'unverified','interactive_playback':'unfinished'},indent=2)+'\n')
print('9 model presentations with retained resources; all original-GPU source readbacks byte-exact')
