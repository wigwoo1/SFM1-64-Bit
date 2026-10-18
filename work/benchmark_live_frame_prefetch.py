from pathlib import Path
import os,subprocess,re,json,statistics,time
records=[]
for repeat in range(3):
 for prefetch in (0,1):
  env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
  env['SFM64_DIAGNOSTIC_FRAME_PREFETCH']=str(prefetch)
  start=time.perf_counter()
  r=subprocess.run(['cmd.exe','/d','/c','work\\run_gpu_only_session_probe.cmd'],env=env,capture_output=True,text=True,timeout=60)
  elapsed=time.perf_counter()-start;log=r.stdout+r.stderr
  Path(f'outputs/live-prefetch-{prefetch}-repeat-{repeat}.txt').write_text(log)
  assert r.returncode==0,log
  assert 'model_gpu_presentation_passed=1 frames=61' in log
  assert 'gpu_readback_copies=0 capture_frames=0 async_output_writes=0' in log
  rows=[]
  for line in log.splitlines():
   if line.startswith('frame_profile '):
    rows.append({k:float(v) for k,v in re.findall(r'(\w+)=([0-9.eE+-]+)',line)})
  assert len(rows)==61
  match=re.search(r'preparation_tasks=(\d+)',log);assert match
  records.append(dict(repeat=repeat,prefetch=prefetch,process_seconds=elapsed,preparation_tasks=int(match[1]),steady_mean_ms={k:statistics.mean(x[k] for x in rows[1:]) for k in rows[0] if k!='frame'},steady_p95_wall_ms=sorted(x['frame_wall_ms'] for x in rows[1:])[56]))
summary=dict(scope='61 hidden GPU0 frames from one real session; excludes editor/native renderer and image readbacks',runs=records,mode_medians={str(mode):statistics.median(r['steady_mean_ms']['frame_wall_ms'] for r in records if r['prefetch']==mode) for mode in (0,1)},limitations='Process startup and shader/pipeline setup are separate from frame timing. Three alternating runs are diagnostic measurements, not a general speedup claim.')
Path('outputs/sfm64-live-prefetch-benchmark.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary,indent=2))
