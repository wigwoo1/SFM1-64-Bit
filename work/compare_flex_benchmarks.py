from pathlib import Path
import json
enabled=json.loads(Path('outputs/scene-worker-benchmark-flex/report.json').read_text(encoding='utf-8'))
disabled=json.loads(Path('outputs/scene-worker-benchmark-flex-disabled/report.json').read_text(encoding='utf-8'))
assert enabled['static_flex'] and not disabled['static_flex']
for field in ('executable_sha256','session_sha256','source_sha256'):
    assert enabled['provenance'][field]==disabled['provenance'][field],field
records=[]
for e,d in zip(enabled['summary'],disabled['summary']):
    assert (e['workers'],e['prefetch'])==(d['workers'],d['prefetch'])
    records.append(dict(workers=e['workers'],prefetch=e['prefetch'],enabled_frame_ms=e['median_frame_wall_ms'],disabled_frame_ms=d['median_frame_wall_ms'],observed_frame_difference_ms=e['median_frame_wall_ms']-d['median_frame_wall_ms'],enabled_cpu_prepare_ms=e['median_cpu_prepare_ms'],disabled_cpu_prepare_ms=d['median_cpu_prepare_ms']))
report=dict(comparisons=records,scope='Same executable/session/source hashes, sequential enabled then disabled runs, two repeats each; uncontrolled scheduling/thermal effects; differences do not isolate flex-stage cost; no native SFM performance comparison')
Path('outputs/sfm64-flex-benchmark-comparison.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(records,indent=2))
