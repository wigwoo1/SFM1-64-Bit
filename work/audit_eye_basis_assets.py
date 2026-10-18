from pathlib import Path
import json,subprocess
assets=json.loads(Path('outputs/sfm64-alternate-loose-assets.json').read_text())['eligible_assets']
records=[]
for index,asset in enumerate(assets):
 base=str(Path(asset['model']).with_suffix(''))
 r=subprocess.run(['build/session-core/inspect_eye_basis.exe',base],capture_output=True,text=True,timeout=30)
 log=r.stdout+r.stderr
 Path(f'outputs/sfm64-eye-basis-asset-{index}.txt').write_text(log)
 records.append(dict(model=asset['model'],passed=r.returncode==0,trace=log))
 assert r.returncode==0,log
Path('outputs/sfm64-eye-basis-assets.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(records,indent=2))
