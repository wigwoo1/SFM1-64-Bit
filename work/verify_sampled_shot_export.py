from pathlib import Path
import hashlib,json
root=Path.cwd();direct=root/'work/shot-render-2470-v1';integrated=root/'work/shot-render-sampled-870833-v1';offline=root/'work/shot-render-offline-sampled-870833-v1'
checks={}
for name in ('camera.bin','model-0-placement.bin','model-0-pose.bin'):
 saved=(direct/name).read_bytes();a=(integrated/name).read_bytes();b=(offline/name).read_bytes()
 assert a==b,name
 checks[name]={'bytes':len(a),'standalone_evaluator_equal':True,'saved_input_changed':a!=saved,'sha256':hashlib.sha256(a).hexdigest()}
assert any(v['saved_input_changed'] for v in checks.values()),'animation did not affect rendered inputs'
manifest=(integrated/'manifest.txt').read_text()
assert 'sampled_executed=499 sampled_changed=62' in manifest
report={'files':checks,'executed_operations':499,'changed_attributes':62,'native_scheduler':'unfinished','animation_scope':'selected-shot recovered subset'}
(root/'outputs/sfm64-sampled-shot-export-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
