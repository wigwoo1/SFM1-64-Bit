import json
from pathlib import Path
root=Path(__file__).resolve().parent.parent
latest=lambda prefix:max((root/'outputs').glob(prefix+'-launch-*'),key=lambda p:p.stat().st_mtime)
single=latest('shadow-flag');off=latest('shadow-all-flags-off');baseline=latest('visible-shadow')
bypass=Path(json.loads((root/'outputs/visible-shadow-comparison-validation.json').read_text())['bypass_receipt']).parent
reports=[json.loads((d/'validation.json').read_text()) for d in (single,off,baseline,bypass)]
assert all(r['source_unchanged'] for r in reports)
previous=Path(json.loads((root/'outputs/visible-shadow-comparison-validation.json').read_text())['shadow_receipt'])
assert reports[2]['byte_exact_rgba_frames']==json.loads(previous.read_text())['byte_exact_rgba_frames']
changed=0
for frame in range(3):
 name=f'vulkan-retained-frame-{frame}.rgba'
 a=(single/'frames-1'/name).read_bytes();b=(baseline/'frames-1'/name).read_bytes();c=(off/'frames-1'/name).read_bytes();unshadowed=(bypass/'frames-1'/name).read_bytes()
 if frame!=1:assert a==b==c
 else:
  assert c==unshadowed and a!=b and a!=unshadowed
  assert a[3::4]==b[3::4]==unshadowed[3::4]
  assert all(b[i]<=a[i]<=unshadowed[i] for i in range(len(a)) if i%4!=3)
  changed=sum(a[i:i+3]!=b[i:i+3] for i in range(0,len(a),4))
 depthname=f'vulkan-scene-shadow-frame-{frame}.depth'
 depth=(single/'frames-1'/depthname).read_bytes();reference=(baseline/'frames-1'/depthname).read_bytes()
 if frame!=1:assert depth==reference
 else:assert depth[:16384]==reference[:16384] and depth[32768:]==reference[32768:]
report=dict(single_receipt=single.as_posix()+'/validation.json',all_off_receipt=off.as_posix()+'/validation.json',baseline_receipt=baseline.as_posix()+'/validation.json',single_flag_rgb_pixels_changed=changed,unaffected_light_depths_unchanged=True,all_off_exact_unshadowed=True,restored_frames_exact=True,alpha_unchanged=True,original_session_unchanged=True,source_shadow_parity=False)
(root/'outputs/shadow-flag-comparison-validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
