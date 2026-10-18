import hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parent.parent
latest=lambda prefix:max((root/'outputs').glob(prefix+'-launch-*'),key=lambda p:p.stat().st_mtime)
shadow=latest('visible-shadow');bypass=latest('visible-shadow-bypass')
reference=json.loads((root/'outputs/scene-shadow-depth-launch-bd7ac455/validation.json').read_text())
shadow_report=json.loads((shadow/'validation.json').read_text());bypass_report=json.loads((bypass/'validation.json').read_text())
assert shadow_report['source_unchanged'] and bypass_report['source_unchanged']
assert bypass_report['byte_exact_rgba_frames']==reference['byte_exact_rgba_frames']
assert shadow_report['byte_exact_shadow_depth_frames']==bypass_report['byte_exact_shadow_depth_frames']==reference['byte_exact_shadow_depth_frames']
frames=[]
for frame in range(3):
 name=f'vulkan-retained-frame-{frame}.rgba'
 a=(shadow/'frames-1'/name).read_bytes();b=(bypass/'frames-1'/name).read_bytes()
 assert len(a)==len(b)==512*512*4 and a[3::4]==b[3::4]
 assert all(a[i]<=b[i] for i in range(len(a)) if i%4!=3)
 changed=sum(a[i:i+3]!=b[i:i+3] for i in range(0,len(a),4));assert changed>0
 frames.append(dict(frame=frame,changed_rgb_pixels=changed,alpha_unchanged=True,rgb_nonincreasing=True))
report=dict(shadow_receipt=shadow.as_posix()+'/validation.json',bypass_receipt=bypass.as_posix()+'/validation.json',bypass_exact_previous_output=True,depth_maps_identical=True,frames=frames,resolution=64,bias=0,filter='nearest',source_shadow_parity=False)
(root/'outputs/visible-shadow-comparison-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
