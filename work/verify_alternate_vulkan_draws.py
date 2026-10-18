from pathlib import Path
import subprocess,os,json,hashlib
records=[]
for model in ['glaceon','lotion cat']:
 env=dict(os.environ)
 for key in list(env):
  if key.startswith('SFM64_DIAGNOSTIC_'):del env[key]
 env.update({'SFM64_DIAGNOSTIC_PRESENT_DEVICE':'0','SFM64_DIAGNOSTIC_PRESENT_MODEL':str(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models')/model),'SFM64_DIAGNOSTIC_CAPTURE_FRAMES':'1'})
 r=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=r.stdout+r.stderr;slug=model.replace(' ','-');Path(f'outputs/sfm64-alternate-vulkan-{slug}-validation.txt').write_text(log)
 record={'model':model,'passed':r.returncode==0,'failure':r.stderr.strip(),'native_visual_parity':'unverified'}
 if r.returncode==0:
  assert 'model_gpu_presentation_passed=1 frames=1' in log
  pixels=Path('outputs/mamagen-vulkan-blended-draw-v1.rgba').read_bytes();Path(f'outputs/{slug}-vulkan-diagnostic-v1.rgba').write_bytes(pixels);record['source_rgba_sha256']=hashlib.sha256(pixels).hexdigest()
 records.append(record)
Path('outputs/sfm64-alternate-vulkan-draw-summary.json').write_text(json.dumps(records,indent=2)+'\n');print(json.dumps(records,indent=2))
