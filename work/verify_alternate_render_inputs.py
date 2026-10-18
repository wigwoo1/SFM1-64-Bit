from pathlib import Path
import subprocess,json
assets=json.loads(Path('outputs/sfm64-alternate-loose-assets.json').read_text())['eligible_assets'];records=[]
for n,a in enumerate(assets):
 r=subprocess.run(['build/session-core/inspect_alternate_render_inputs.exe',str(Path(a['model']).with_suffix(''))],capture_output=True,text=True,timeout=45)
 Path(f'outputs/sfm64-alternate-render-inputs-{n}.txt').write_text(r.stdout+r.stderr)
 records.append({'model':a['model'],'passed':r.returncode==0,'failure':r.stderr.strip()})
Path('outputs/sfm64-alternate-render-inputs-summary.json').write_text(json.dumps(records,indent=2)+'\n');print(json.dumps(records,indent=2))
