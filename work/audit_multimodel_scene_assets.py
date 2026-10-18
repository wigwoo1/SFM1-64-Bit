from pathlib import Path
import subprocess,json,re
game=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
mounts=[game/'usermod']+sorted(p for p in game.iterdir() if p.is_dir() and p.name!='usermod')
records=[];shot=None
for line in Path('outputs/sfm64-session-render-scenes-validation.txt').read_text().splitlines():
 m=re.match(r'shot=(\d+) models=',line)
 if m:shot=int(m[1]);continue
 if shot not in (20053,32107):continue
 m=re.match(r' model=(\d+) visible=(\d+) skin=(\d+) body=(\d+) asset=(.+)',line)
 if not m:continue
 asset=m[5];relative=Path(asset)
 assert not relative.is_absolute() and '..' not in relative.parts
 candidates=[p/relative for p in mounts if (p/relative).is_file()]
 record=dict(shot=shot,id=int(m[1]),asset=asset,loose_candidates=[str(p) for p in candidates])
 if not candidates:record['failure']='no loose MDL in inspected game mounts'
 else:
  base=str(candidates[0].with_suffix(''))
  for kind,exe in [('geometry','inspect_alternate_geometry'),('materials','inspect_alternate_render_inputs')]:
   result=subprocess.run([f'build/session-core/{exe}.exe',base],capture_output=True,text=True,timeout=30)
   log=result.stdout+result.stderr;Path(f'outputs/multi-shot-{shot}-model-{m[1]}-{kind}.txt').write_text(log)
   record[kind]=dict(passed=result.returncode==0,failure=result.stderr.strip())
 records.append(record)
assert len(records)==7
Path('outputs/sfm64-multimodel-scene-assets.json').write_text(json.dumps(dict(scope='loose files only; mount priority is diagnostic, not recovered SFM precedence',models=records),indent=2)+'\n')
print(json.dumps(records,indent=2))
