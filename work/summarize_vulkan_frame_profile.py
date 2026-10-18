from pathlib import Path
import json,math,statistics
summary=[]
for mode in (0,1):
 rows=[]
 for line in Path(f'outputs/sfm64-vulkan-frame-prefetch-mode-{mode}.txt').read_text().splitlines():
  if not line.startswith('frame_profile '):continue
  row={key:float(value) for key,value in (item.split('=',1) for item in line.split()[1:])}
  assert all(math.isfinite(v) and v>=0 for v in row.values());rows.append(row)
 assert len(rows)==9
 warm=rows[1:]
 summary.append({'prefetch':bool(mode),'frames':rows,'warm_frame_mean_ms':{key:statistics.mean(row[key] for row in warm) for key in warm[0] if key!='frame'}})
report={'measurements':summary,'scope':'nine-frame offscreen fixture diagnostic, includes readback and file output','gpu_time':'not measured; fence wait is host wait','claim':'single run observation, not whole-application benchmark'}
Path('outputs/sfm64-vulkan-frame-profile-summary.json').write_text(json.dumps(report,indent=2)+'\n')
for row in summary:print('prefetch='+str(row['prefetch'])+' '+json.dumps(row['warm_frame_mean_ms']))
