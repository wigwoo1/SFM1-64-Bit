from pathlib import Path
import csv,json,statistics,hashlib
rows=list(csv.DictReader(Path('outputs/flex-buffer-reuse-profile.csv').open(encoding='utf-8')))
assert len(rows)==1440
frames={}
for row in rows:
 frame=int(row['frame']);reuse=int(row['reuse'])
 if frame<30:continue
 sums=frames.setdefault((frame,reuse),dict(copy_ms=0.,processing_ms=0.,deformation_ms=0.))
 for field in sums:sums[field]+=float(row[field])
assert len(frames)==300
summary={str(reuse):{field:statistics.median(v[field] for (f,r),v in frames.items() if r==reuse) for field in ('copy_ms','processing_ms','deformation_ms')} for reuse in (0,1)}
report=dict(median_frame_stage_sums_ms=summary,median_paired_savings_ms={field:statistics.median(frames[(f,0)][field]-frames[(f,1)][field] for f in range(30,180)) for field in summary['0']},scope='180 CPU four-model frames, first 30 omitted; same inputs fresh/reused with alternating call order, exact output and stable reuse pointers checked; warmed caches; not end-to-end renderer or native SFM comparison')
report['sha256']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (Path('work/model_flex_deformation_candidate.h'),Path('work/retained_static_flex_provider_candidate.h'),Path('work/profile_flex_buffer_reuse.cpp'),Path('build/session-core/profile_flex_buffer_reuse.exe'),Path('work/roundtrip-11.dmx'))}
Path('outputs/flex-buffer-reuse-profile-summary.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(summary,indent=2))
