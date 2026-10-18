from pathlib import Path
import json
records=[]
for scene in ('static-flex','four-model-static-flex'):
 before=json.loads(Path(f'outputs/sfm64-{scene}-before-borrowed-output.json').read_text(encoding='utf-8'))
 after=json.loads(Path(f'outputs/sfm64-{scene}-gpu-validation.json').read_text(encoding='utf-8'))
 assert len(before['runs'])==len(after['runs'])==5
 for a,b in zip(before['runs'],after['runs']):
  for key in ('enabled','workers','prefetch','frame_sha256','changed_pixels_vs_disabled'):
   assert a[key]==b[key],(scene,key)
 records.append(dict(scene=scene,runs=5,frames_per_run=62,all_saved_hashes_exact=True))
Path('outputs/sfm64-borrowed-flex-reference-validation.json').write_text(json.dumps(dict(scenes=records,scope='Candidate Vulkan output before/after retained flex storage; no native parity claim'),indent=2)+'\n',encoding='utf-8')
print('Borrowed flex output matches all 620 saved GPU frame hashes')
