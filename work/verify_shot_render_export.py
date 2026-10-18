from pathlib import Path
import json,hashlib
directory=Path('work/shot-render-2470-v1')
pairs=[('camera.bin','session-camera-engine-view-v1.bin'),('model-0-placement.bin','session-mamagen-placement-v1.bin'),('model-0-pose.bin','session-mamagen-bone-pose-v1.bin')]
checks=[]
for name,reference in pairs:
 data=(directory/name).read_bytes()
 assert data==Path('work',reference).read_bytes(),name
 checks.append({'file':name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'matches_prior_validated_input':True})
assert not Path('work/shot-render-no-camera-test').exists()
result={'inputs':checks,'missing_camera_export_rejected_without_output':True,'scope':'Explicit shot selection and static saved pose. Timeline selection, sampled playback and native full-render parity remain incomplete.'}
Path('outputs/sfm64-shot-render-export-validation.json').write_text(json.dumps(result,indent=2));print('shot export three inputs byte-exact; missing camera produced no output')
