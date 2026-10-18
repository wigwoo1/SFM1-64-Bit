import csv, json, statistics
from pathlib import Path
rows=list(csv.DictReader(Path('outputs/sfm64-preparation-stage-profile.csv').open()))
report={}
for workers in ('1','4'):
    selected=[r for r in rows if r['workers']==workers and int(r['frame'])>=30]
    assert len(selected)==150
    report[workers]={key:statistics.median(float(r[key]) for r in selected) for key in selected[0] if key.endswith('_ms')}
result=dict(scope='CPU-only four-model retained sampling, 180 frames per worker count, first 30 omitted; one run per configuration; no native parity or GPU timing claim',median_ms=report)
Path('outputs/sfm64-preparation-stage-profile.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result,indent=2))
