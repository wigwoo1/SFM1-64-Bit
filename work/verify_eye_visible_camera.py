from pathlib import Path
import os, subprocess, struct, json, hashlib
camera=Path('work/eye-visible-camera.bin')
camera.write_bytes(struct.pack('<16f',.02,0,0,0,0,0,-.02,.8,0,.01,0,.5,0,0,0,1))
records=[]
for enabled,skin in [(False,0),(True,0),(True,1),(True,2)]:
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\glaceon',SFM64_DIAGNOSTIC_MODEL_SKIN=str(skin),SFM64_DIAGNOSTIC_EYE_CANDIDATE=str(int(enabled)),SFM64_DIAGNOSTIC_CAMERA_MATRIX=str(camera.resolve()))
 r=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=r.stdout+r.stderr;name=f'eye-visible-{int(enabled)}-skin-{skin}'
 Path(f'outputs/{name}.txt').write_text(log)
 assert r.returncode==0,log
 assert f'eye_candidate_enabled={int(enabled)} eye_draw_calls={2 if enabled else 0}' in log
 pixels=Path('outputs/mamagen-vulkan-session-camera-v1.rgba').read_bytes()
 Path(f'outputs/{name}.rgba').write_bytes(pixels)
 records.append(dict(enabled=enabled,skin=skin,sha256=hashlib.sha256(pixels).hexdigest(),native_parity='unverified'))
 from PIL import Image
 Image.frombytes('RGBA',(512,512),pixels).save(f'outputs/{name}.png')
Path('outputs/sfm64-eye-visible-camera-summary.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(records,indent=2))
assert len({r['sha256'] for r in records[1:]})==3,'visible iris variants unchanged'
assert records[0]['sha256']!=records[1]['sha256'],'eye candidate did not affect visible pixels'
