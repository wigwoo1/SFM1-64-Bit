from pathlib import Path
import json
root=Path('work/retained-shot-sequence-v1');checks=[]
for frame,reference in ((0,'shot-render-sampled-870833-v1'),(1,'shot-render-sampled-880833-v1'),(2,'shot-render-sampled-870833-v1')):
 for name in ('camera.bin','model-0-placement.bin','model-0-pose.bin'):
  a=(root/f'frame-{frame}'/name).read_bytes();b=(Path('work')/reference/name).read_bytes();assert a==b,(frame,name)
  checks.append({'frame':frame,'file':name,'fresh_serial_equal':True,'bytes':len(a)})
 manifest=(root/f'frame-{frame}'/'manifest.txt').read_text();assert f'rebuilt={int(frame==0)}' in manifest
Path('outputs/sfm64-retained-shot-sequence-byte-validation.json').write_text(json.dumps({'checks':checks,'scalar_workers':2,'vector_workers':2,'frames':3,'prepared_rebuilds':1,'backwards_seek_equal':True,'native_animation_parity':'unverified'},indent=2)+'\n')
print('retained parallel shot export: 9 binary files byte-exact with fresh serial frames; backward seek verified')
