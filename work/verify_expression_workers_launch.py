"""Real native expression worker selection and bounded frame parity; no Blender."""
import hashlib,json,os,re,subprocess,uuid
from pathlib import Path
root=Path(__file__).resolve().parent.parent
exe=root/'build/session-core/probe_vulkan_win32_surface_revision_x64.exe'
source=root/'work/short-timeline-session.dmx'
before=hashlib.sha256(source.read_bytes()).hexdigest()
output=root/'outputs'/('expression-worker-launch-'+uuid.uuid4().hex[:8]);output.mkdir()
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1')
session=['--session',str(source),'--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test']
for workers in (0,2):
 frames=output/f'frames-{workers}';frames.mkdir()
 worker_env=dict(env,SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(frames))
 result=subprocess.run([str(exe),*session,'--expression-workers',str(workers)],cwd=root,env=worker_env,capture_output=True,text=True,timeout=120)
 log=result.stdout+result.stderr;(output/f'workers-{workers}.txt').write_text(log)
 assert result.returncode==0,log
 if workers:
  jobs=[int(value) for value in re.findall(r'scene_private_expression_jobs=(\d+)',log)]
  assert jobs and max(jobs)>0,log
frame_hashes={}
for frame in range(3):
 name=f'vulkan-retained-frame-{frame}.rgba'
 first=(output/'frames-0'/name).read_bytes()
 second=(output/'frames-2'/name).read_bytes()
 assert len(first)==512*512*4 and first==second,name
 assert len(set(first))>4,'Empty or constant frame: '+name
 frame_hashes[name]=hashlib.sha256(first).hexdigest()
assert len(set(frame_hashes.values()))>1,'Fixture did not exercise changing frames'
cases=[(['--expression-workers'],'requires one count'),(['--expression-workers','61'],'must be 0..60'),(['--expression-workers','65'],'must be 0..60'),(['--expression-workers','2','--expression-workers','3'],'requires one count'),(['--expression-workers','2'],'require a session')]
for args,message in cases:
 result=subprocess.run([str(exe),*args],cwd=root,env=env,capture_output=True,text=True,timeout=10)
 assert result.returncode==1 and message in result.stdout+result.stderr,(args,result)
conflict=dict(env,SFM64_DIAGNOSTIC_EXPRESSION_WORKERS='3')
result=subprocess.run([str(exe),*session,'--expression-workers','2'],cwd=root,env=conflict,capture_output=True,text=True,timeout=10)
assert result.returncode==1 and 'conflicts with diagnostic setting' in result.stdout+result.stderr
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
report=dict(real_session_expression_worker_counts=[0,2],invalid_or_conflicting_cases=6,source_unchanged=True,
 byte_exact_rgba_frames=frame_hashes,frame_resolution=[512,512],
 scope='Actual native serial/private parallel expression selection, nonzero private expression jobs and three Source-session frame comparisons; speedup, other scenes and optimal worker count unverified.')
(output/'validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
