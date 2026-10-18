from pathlib import Path
import csv,json,statistics,hashlib
rows=list(csv.DictReader(Path('outputs/flex-stage-profile.csv').open(encoding='utf-8')))
assert len(rows)==960
stages=('metadata_ms','controllers_ms','rules_ms','deformation_ms','copy_ms','processing_ms')
frames={}
for r in rows:
    frame=int(r['frame'])
    if frame<30:continue
    totals=frames.setdefault(frame,{s:0. for s in stages})
    for s in stages:totals[s]+=float(r[s])
assert len(frames)==210
report=dict(frame_sum_medians_ms={s:statistics.median(f[s] for f in frames.values()) for s in stages},models={})
for model in sorted({r['model'] for r in rows}):
    matching=[r for r in rows if r['model']==model and int(r['frame'])>=30]
    report['models'][model]={s:statistics.median(float(r[s]) for r in matching) for s in stages}
report['scope']='CPU four-model scene, 240 frames, first 30 omitted; optional stage instrumentation; deformation includes geometry copy, body selection, quantization and accumulation; no native comparison'
report['sha256']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (Path('work/retained_static_flex_provider_candidate.h'),Path('work/model_flex_deformation_candidate.h'),Path('build/session-core/profile_flex_stages.exe'),Path('work/roundtrip-11.dmx'))}
Path('outputs/flex-stage-profile-summary.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report['frame_sum_medians_ms'],indent=2))
