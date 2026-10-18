from pathlib import Path
import subprocess,os,json,hashlib,re
records=[]
for skin,body in [(0,0),(1,0),(2,0),(0,1),(0,2)]:
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update({'SFM64_DIAGNOSTIC_PRESENT_DEVICE':'0','SFM64_DIAGNOSTIC_PRESENT_MODEL':r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\glaceon','SFM64_DIAGNOSTIC_MODEL_SKIN':str(skin),'SFM64_DIAGNOSTIC_MODEL_BODY':str(body)})
 r=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=r.stdout+r.stderr;Path(f'outputs/sfm64-glaceon-skin-{skin}-body-{body}-validation.txt').write_text(log);assert r.returncode==0,log
 assert 'model_gpu_presentation_passed=1 frames=1' in log
 triangles=int(re.search(r'model_selection skin=\d+ body=\d+ triangles=(\d+)',log)[1])
 pixels=Path('outputs/mamagen-vulkan-blended-draw-v1.rgba').read_bytes();Path(f'outputs/glaceon-skin-{skin}-body-{body}.rgba').write_bytes(pixels)
 records.append({'skin':skin,'body':body,'triangles':triangles,'source_pixel_sha256':hashlib.sha256(pixels).hexdigest(),'native_visual_parity':'unverified'})
assert records[0]['triangles']!=records[3]['triangles']
assert records[0]['source_pixel_sha256']!=records[3]['source_pixel_sha256']
Path('outputs/sfm64-alternate-selection-vulkan-summary.json').write_text(json.dumps(records,indent=2)+'\n');print(json.dumps(records,indent=2))
