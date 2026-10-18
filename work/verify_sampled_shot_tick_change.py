from pathlib import Path
from PIL import Image
import json
a=Path('work/shot-render-sampled-870833-v1');b=Path('work/shot-render-sampled-880833-v1')
changed={name:(a/name).read_bytes()!=(b/name).read_bytes() for name in ('camera.bin','model-0-placement.bin','model-0-pose.bin')}
assert changed['model-0-pose.bin'],'bone pose unchanged across ticks'
first=Path('outputs/mamagen-vulkan-sampled-shot-v1.rgba').read_bytes();second=Path('outputs/mamagen-vulkan-session-camera-v1.rgba').read_bytes()
assert len(first)==len(second)==512*512*4
pixels=sum(first[n:n+4]!=second[n:n+4] for n in range(0,len(first),4))
assert pixels>0,'render did not change across ticks'
Path('outputs/mamagen-vulkan-sampled-shot-second-tick-v1.rgba').write_bytes(second)
Image.frombytes('RGBA',(512,512),second).save('outputs/mamagen-vulkan-sampled-shot-second-tick-v1.png')
report={'root_parent_ticks':[870833,880833],'shot':2470,'input_changed':changed,'changed_render_pixels':pixels,'native_animation_parity':'unverified','frame_execution':'separate offscreen diagnostic runs'}
Path('outputs/sfm64-sampled-shot-tick-change-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
