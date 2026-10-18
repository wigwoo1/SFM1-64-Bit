"""Historical comparisons expose repeat spread; they are not controlled A/B tests."""
from pathlib import Path
import json
before_path=Path('outputs/scene-worker-benchmark-reserved/report.json')
after_path=Path('outputs/scene-worker-benchmark-parallel-placement/report.json')
before=json.loads(before_path.read_text(encoding='utf-8'))
after=json.loads(after_path.read_text(encoding='utf-8'))
assert before['provenance']['session_sha256']==after['provenance']['session_sha256']
rows=[]
for old in before['summary']:
    matches=lambda r:r['workers']==old['workers'] and r['prefetch']==old['prefetch']
    new=next(r for r in after['summary'] if matches(r))
    values=lambda report:[r['metrics_ms']['frame_wall_ms']['median'] for r in report['runs'] if matches(r)]
    rows.append(dict(workers=old['workers'],prefetch=old['prefetch'],
        before_median_ms=old['median_frame_wall_ms'],after_median_ms=new['median_frame_wall_ms'],
        before_run_medians_ms=values(before),after_run_medians_ms=values(after),
        observed_reduction_percent=100*(1-new['median_frame_wall_ms']/old['median_frame_wall_ms'])))
report=dict(before=str(before_path),after=str(after_path),rows=rows,
    interpretation='Same saved session, separate historical runs on an uncontrolled machine; worker error-slot and parallel placement changes both occurred between reports. Two repeats cannot isolate causality or establish broad/native SFM speedup.')
Path('outputs/sfm64-parallel-placement-benchmark-comparison.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(rows,indent=2))
