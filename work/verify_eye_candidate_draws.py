from pathlib import Path
import subprocess, os, hashlib, json
records=[]
for enabled,skin in [(False,0),(True,0),(True,1),(True,2)]:
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\glaceon',SFM64_DIAGNOSTIC_MODEL_SKIN=str(skin),SFM64_DIAGNOSTIC_EYE_CANDIDATE=str(int(enabled)))
 r=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
 log=r.stdout+r.stderr
 name=f'eye-candidate-{int(enabled)}-skin-{skin}'
 Path(f'outputs/{name}.txt').write_text(log)
 assert r.returncode==0,log
 assert 'model_gpu_presentation_passed=1 frames=1' in log
 assert f'eye_candidate_enabled={int(enabled)} eye_draw_calls={2 if enabled else 0}' in log,log
 pixels=Path('outputs/mamagen-vulkan-blended-draw-v1.rgba').read_bytes()
 Path(f'outputs/{name}.rgba').write_bytes(pixels)
 records.append(dict(enabled=enabled,skin=skin,sha256=hashlib.sha256(pixels).hexdigest(),native_parity='unverified'))
 assert len(pixels)==512*512*4
 if not enabled:
  assert pixels==Path('outputs/glaceon-skin-0-body-0.rgba').read_bytes(),'disabled baseline changed'
Path('outputs/sfm64-eye-candidate-draw-summary.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(records,indent=2))
assert len({r['sha256'] for r in records[1:]})==3,'iris variants did not change pixels'
assert records[0]['sha256']!=records[1]['sha256']
