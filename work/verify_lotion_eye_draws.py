from pathlib import Path
import os, subprocess, struct, json, hashlib,re
from PIL import Image
camera=Path('work/lotion-eye-camera.bin')
camera.write_bytes(struct.pack('<16f',.02,0,0,0,0,0,-.02,.8,0,.01,0,.6,0,0,0,1))
records=[];captures=[]
for enabled,skin in [(False,0),(True,0),(True,1),(True,2)]:
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\lotion cat',SFM64_DIAGNOSTIC_MODEL_SKIN=str(skin),SFM64_DIAGNOSTIC_EYE_CANDIDATE=str(int(enabled)),SFM64_DIAGNOSTIC_CAMERA_MATRIX=str(camera.resolve()))
 r=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=r.stdout+r.stderr;name=f'lotion-eye-{int(enabled)}-skin-{skin}'
 Path(f'outputs/{name}.txt').write_text(log)
 assert r.returncode==0,log
 assert f'eye_candidate_enabled={int(enabled)} eye_draw_calls={2 if enabled else 0}' in log
 pixels=Path('outputs/mamagen-vulkan-session-camera-v1.rgba').read_bytes();captures.append(pixels)
 Path(f'outputs/{name}.rgba').write_bytes(pixels)
 Image.frombytes('RGBA',(512,512),pixels).save(f'outputs/{name}.png')
 changed=sum(pixels[i:i+4]!=captures[0][i:i+4] for i in range(0,len(pixels),4))
 records.append(dict(enabled=enabled,skin=skin,changed_pixels_from_disabled=changed,sha256=hashlib.sha256(pixels).hexdigest(),native_parity='unverified',eye_bounds=re.findall(r'eye_draw_bounds[^\n]+',log)))
Path('outputs/sfm64-lotion-eye-draw-summary.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(records,indent=2))
assert len({r['sha256'] for r in records[1:]})==3,'visible iris variants unchanged'
assert all(r['changed_pixels_from_disabled']>0 for r in records[1:]),'eye candidate did not affect visible pixels'
