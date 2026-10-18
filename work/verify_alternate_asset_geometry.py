from pathlib import Path
import subprocess,json,re
assets=json.loads(Path('outputs/sfm64-alternate-loose-assets.json').read_text())['eligible_assets'];records=[]
for n,asset in enumerate(assets):
 base=str(Path(asset['model']).with_suffix(''));r=subprocess.run(['build/session-core/inspect_alternate_geometry.exe',base],capture_output=True,text=True,timeout=30)
 log=r.stdout+r.stderr;Path(f'outputs/sfm64-alternate-geometry-{n}-validation.txt').write_text(log)
 records.append({'model':asset['model'],'passed':r.returncode==0,'skin_families':asset['skin_families'],'body_variants':sum(asset['body_model_counts']),'failure':r.stderr.strip()})
Path('outputs/sfm64-alternate-geometry-summary.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(records,indent=2))
