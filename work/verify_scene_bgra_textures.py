from pathlib import Path
import json,subprocess,hashlib
textures=json.loads(Path('outputs/sfm64-scene-texture-formats.json').read_text())['textures'];records=[]
for t in (x for x in textures if x['format']==12):
 output=Path('outputs/bgra8888-validation-scratch.rgba')
 r=subprocess.run(['build/session-core/test_iris_texture.exe',t['path'],str(output)],capture_output=True,text=True,timeout=30)
 record=dict(path=t['path'],passed=r.returncode==0,log=r.stdout+r.stderr)
 if r.returncode==0:
  data=Path(t['path']).read_bytes();pixels=output.read_bytes()
  assert len(pixels)==t['width']*t['height']*4
  expected=bytearray(data[-len(pixels):]);expected[0::4],expected[2::4]=expected[2::4],expected[0::4]
  assert pixels==expected,'BGRA8888 channel conversion mismatch'
  record['pixel_sha256']=hashlib.sha256(pixels).hexdigest()
 records.append(record)
Path('outputs/sfm64-bgra8888-texture-validation.json').write_text(json.dumps(records,indent=2)+'\n')
print('BGRA8888 textures:',len(records),'passed:',sum(r['passed'] for r in records))
assert len(records)==5 and all(r['passed'] for r in records)
