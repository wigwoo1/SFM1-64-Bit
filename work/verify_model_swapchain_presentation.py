from pathlib import Path
import json
log=Path('outputs/sfm64-model-swapchain-presentation-validation.txt').read_text()
assert log.count('model_gpu_presentation_passed=1 frames=3')==2
assert log.count('source=model_gpu_image frames=1')==6
assert log.count('present_result=0')==12
checks=[]
for frame,reference in enumerate(('mamagen-vulkan-sampled-shot-v1.rgba','mamagen-vulkan-sampled-shot-second-tick-v1.rgba','mamagen-vulkan-sampled-shot-v1.rgba')):
 actual=Path(f'outputs/vulkan-retained-frame-{frame}.rgba').read_bytes();expected=Path('outputs',reference).read_bytes();assert len(actual)==len(expected)==512*512*4
 differences=[abs(a-b) for a,b in zip(actual,expected)]
 checks.append({'frame':frame,'last_device_render_readback_exact':actual==expected,'different_components':sum(d!=0 for d in differences),'max_component_difference':max(differences)})
Path('outputs/sfm64-model-swapchain-presentation-summary.json').write_text(json.dumps({'devices_presented':2,'model_presentations':6,'render_readback_checks':checks,'presentation_transfer':'GPU image blit','window_visible':False,'swapchain_visual_pixels':'unverified','presentation_sync':'serialized queue idle','draw_sync':'submission fence','interactive_playback':'unfinished'},indent=2)+'\n')
print(json.dumps(checks));print('two devices presented 3 GPU model images each; cross-device readback differences recorded')
