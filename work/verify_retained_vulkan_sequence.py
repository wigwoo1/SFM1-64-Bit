from pathlib import Path
import json,hashlib
references=('mamagen-vulkan-sampled-shot-v1.rgba','mamagen-vulkan-sampled-shot-second-tick-v1.rgba','mamagen-vulkan-sampled-shot-v1.rgba')
checks=[]
for n,name in enumerate(references):
 raw=Path(f'outputs/vulkan-retained-frame-{n}.rgba').read_bytes();reference=Path('outputs',name).read_bytes()
 assert len(raw)==512*512*4 and raw==reference,(n,name)
 checks.append({'frame':n,'independent_render_byte_exact':True,'sha256':hashlib.sha256(raw).hexdigest()})
log=Path('outputs/sfm64-retained-vulkan-sequence-validation.txt').read_text()
assert 'retained Vulkan sequence passed frames=3' in log
assert log.count('Vulkan indexed model draw passed')==3
assert 'gpu_wait=submission_fence fence_waits=3 queue_idle_waits=0' in log
Path('outputs/sfm64-retained-vulkan-pixel-validation.json').write_text(json.dumps({'frames':checks,'resource_reuse_scope':'single model offscreen sequence','gpu_wait':'submission fence per frame','queue_idle_waits_normal_draw':0,'native_render_parity':'unverified'},indent=2)+'\n')
print('three retained Vulkan frames match independent renders byte for byte, including backward seek')
