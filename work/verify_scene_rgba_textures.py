from pathlib import Path
import json,subprocess,hashlib
textures=json.loads(Path('outputs/sfm64-scene-texture-formats.json').read_text())['textures'];records=[]
for n,t in enumerate(x for x in textures if x['format']==0):
 output=Path('outputs/rgba8888-validation-scratch.rgba')
 r=subprocess.run(['build/session-core/test_iris_texture.exe',t['path'],str(output)],capture_output=True,text=True,timeout=30)
 record=dict(path=t['path'],passed=r.returncode==0,log=r.stdout+r.stderr)
 if r.returncode==0:
  data=Path(t['path']).read_bytes();pixels=output.read_bytes()
  assert len(pixels)==t['width']*t['height']*4
  assert pixels==data[-len(pixels):],'RGBA8888 highest mip byte mismatch'
  record['pixel_sha256']=hashlib.sha256(pixels).hexdigest()
 records.append(record)
Path('outputs/sfm64-rgba8888-texture-validation.json').write_text(json.dumps(records,indent=2)+'\n')
print('RGBA8888 textures:',len(records),'passed:',sum(r['passed'] for r in records))
assert all(r['passed'] for r in records)
