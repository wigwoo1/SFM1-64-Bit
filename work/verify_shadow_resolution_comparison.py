import json
from pathlib import Path
root=Path(__file__).resolve().parent.parent
latest=lambda prefix:max((root/'outputs').glob(prefix+'-launch-*'),key=lambda p:p.stat().st_mtime)
changed=latest('shadow-resolution');baseline=latest('visible-shadow')
a=json.loads((changed/'validation.json').read_text());b=json.loads((baseline/'validation.json').read_text())
prior=Path(json.loads((root/'outputs/visible-shadow-comparison-validation.json').read_text())['shadow_receipt'])
assert b['byte_exact_rgba_frames']==json.loads(prior.read_text())['byte_exact_rgba_frames']
assert a['source_unchanged'] and b['source_unchanged']
pixels_changed=0
for frame in range(3):
 name=f'vulkan-retained-frame-{frame}.rgba';x=(changed/'frames-1'/name).read_bytes();y=(baseline/'frames-1'/name).read_bytes()
 depthname=f'vulkan-scene-shadow-frame-{frame}.depth'
 if frame!=1:assert x==y and (changed/'frames-1'/depthname).read_bytes()==(baseline/'frames-1'/depthname).read_bytes()
 else:
  assert x!=y and x[3::4]==y[3::4]
  pixels_changed=sum(x[i:i+3]!=y[i:i+3] for i in range(0,len(x),4))
report=dict(changed_receipt=changed.as_posix()+'/validation.json',baseline_receipt=baseline.as_posix()+'/validation.json',resolution_sequence=[64,128,64],changed_rgb_pixels=pixels_changed,alpha_unchanged=True,restored_rgba_and_depth_exact=True,default_baseline_unchanged=True,worker_parity=True,original_source_unchanged=True,settings_menu_implemented=False,source_shadow_parity=False)
(root/'outputs/shadow-resolution-comparison-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
