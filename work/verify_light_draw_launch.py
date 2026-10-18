"""Real native session worker selection and bounded frame parity; no Blender."""
import hashlib,json,math,os,re,subprocess,uuid
from pathlib import Path
root=Path(__file__).resolve().parent.parent
exe=root/'build/session-core/probe_vulkan_win32_surface_revision_x64.exe'
source=root/'work/short-timeline-session.dmx'
before=hashlib.sha256(source.read_bytes()).hexdigest()
output=root/'outputs'/('light-draw-launch-'+uuid.uuid4().hex[:8]);output.mkdir()
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_LIGHT_BUFFER_TEST='1',SFM64_DIAGNOSTIC_LIGHT_DESCRIPTOR_TEST='1',SFM64_DIAGNOSTIC_LIGHT_PIPELINE_TEST='1',SFM64_DIAGNOSTIC_LIGHT_DRAW_TEST='1',SFM64_DIAGNOSTIC_LIGHT_DRAW_GAIN='1000000',SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1')
session=['--session',str(source),'--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test']
stage_samples={}
for workers in (1,2):
 frames=output/f'frames-{workers}';frames.mkdir()
 worker_env=dict(env,SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(frames))
 result=subprocess.run([str(exe),*session,'--geometry-workers',str(workers)],cwd=root,env=worker_env,capture_output=True,text=True,timeout=120)
 log=result.stdout+result.stderr;(output/f'workers-{workers}.txt').write_text(log)
 assert result.returncode==0 and f'geometry_worker_count={workers} range_policy=' in log,log
 assert f'retained_geometry_workers={workers}' in log and 'projected_light_buffer_device_test=passed' in log and 'projected_light_shader_readback=passed' in log and 'light_descriptor_device_test=passed' in log and 'gobo_device_upload_readback=passed' in log and 'session_light_constants_gpu_readback=passed' in log and 'projected_light_graphics_pipeline_device_test=passed' in log and 'projected_light_draws=' in log,log
 profiles=[]
 for line in log.splitlines():
  if line.startswith('scene_profile '):
   fields=dict(token.split('=',1) for token in line.split()[1:])
   values={key:float(fields[key]) for key in ('evaluation_ms','poses_ms','geometry_ms','materials_ms','upload_preparation_ms','decoration_ms')}
   assert all(math.isfinite(value) and value>=0 for value in values.values()),line
   profiles.append(dict(frame=int(fields['frame']),sample_scope=fields['sample_scope'],**values))
 assert len(profiles)==5 and profiles[0]['sample_scope']=='bootstrap',log
 assert all(profile['sample_scope']=='current' for profile in profiles[1:]),profiles
 stage_samples[str(workers)]=profiles
frame_hashes={}
for frame in range(3):
 name=f'vulkan-retained-frame-{frame}.rgba'
 first=(output/'frames-1'/name).read_bytes()
 second=(output/'frames-2'/name).read_bytes()
 assert len(first)==512*512*4 and first==second,name
 assert len(set(first))>4,'Empty or constant frame: '+name
 frame_hashes[name]=hashlib.sha256(first).hexdigest()
assert len(set(frame_hashes.values()))>1,'Fixture did not exercise changing frames'
cases=[(['--geometry-workers'],'requires one count'),(['--geometry-workers','0'],'must be 1..64'),(['--geometry-workers','65'],'must be 1..64'),(['--geometry-workers','2','--geometry-workers','3'],'requires one count'),(['--geometry-workers','2'],'require a session')]
for args,message in cases:
 result=subprocess.run([str(exe),*args],cwd=root,env=env,capture_output=True,text=True,timeout=10)
 assert result.returncode==1 and message in result.stdout+result.stderr,(args,result)
conflict=dict(env,SFM64_DIAGNOSTIC_GEOMETRY_WORKERS='3')
result=subprocess.run([str(exe),*session,'--geometry-workers','2'],cwd=root,env=conflict,capture_output=True,text=True,timeout=10)
assert result.returncode==1 and 'conflicts with diagnostic setting' in result.stdout+result.stderr
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
report=dict(real_session_worker_counts=[1,2],invalid_or_conflicting_cases=6,source_unchanged=True,
 byte_exact_rgba_frames=frame_hashes,frame_resolution=[512,512],
 scene_stage_samples_ms=stage_samples,
 scope='Actual device storage buffer allocation and host-mapped byte verification, plus native frame regression; three-record compute shader field readback bit-identical; actual material/light descriptor pool creation and writes pass; actual installed gobo image upload/copy-readback byte-exact; actual first-shot light constants shader-readback exact; projected light graphics pipeline device-created; actual projected-light draws at diagnostic gain1000000; Source lighting parity unverified.')
(output/'validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
