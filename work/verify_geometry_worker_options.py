from pathlib import Path
import subprocess,os,json,re,statistics
records=[]
for workers in ('1','2','4','8','0','65','4x'):
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env['SFM64_DIAGNOSTIC_GEOMETRY_WORKERS']=workers
 r=subprocess.run(['cmd.exe','/d','/c','work\\run_gpu_only_session_probe.cmd'],env=env,capture_output=True,text=True,timeout=60)
 log=r.stdout+r.stderr;Path(f'outputs/geometry-workers-{workers}.txt').write_text(log)
 if workers in ('0','65','4x'):
  assert r.returncode!=0 and 'geometry workers must be 1..64' in log,log
  records.append(dict(workers=workers,rejected=True));continue
 assert r.returncode==0,log
 assert f'geometry_worker_count={workers} range_policy=contiguous' in log
 assert 'model_gpu_presentation_passed=1 frames=61' in log
 times=[float(x) for x in re.findall(r'frame_wall_ms=([0-9.eE+-]+)',log)]
 assert len(times)==61
 records.append(dict(workers=int(workers),frames=61,steady_mean_wall_ms=statistics.mean(times[1:]),pixel_verification='disabled',performance_scope='one diagnostic run; no speedup conclusion'))
Path('outputs/sfm64-geometry-worker-options.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(records,indent=2))
