"""Real native session worker selection and bounded frame parity; no Blender."""
import hashlib,json,math,os,re,subprocess,uuid
from pathlib import Path
root=Path(__file__).resolve().parent.parent
exe=root/'build/session-core/probe_vulkan_win32_surface_revision_x64.exe'
source=root/'work/short-timeline-session.dmx'
before=hashlib.sha256(source.read_bytes()).hexdigest()
output=root/'outputs'/('light-capacity-256-launch-'+uuid.uuid4().hex[:8]);output.mkdir()
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_LIGHT_BUFFER_TEST='1',SFM64_DIAGNOSTIC_LIGHT_DESCRIPTOR_TEST='1',SFM64_DIAGNOSTIC_LIGHT_PIPELINE_TEST='1',SFM64_DIAGNOSTIC_LIGHT_DRAW_TEST='1',SFM64_DIAGNOSTIC_LIGHT_COUNT_CONTROL='256',SFM64_DIAGNOSTIC_LIGHT_DRAW_GAIN='1',SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1')
session=['--session',str(source),'--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test']
stage_samples={}
for workers in (1,2):
 frames=output/f'frames-{workers}';frames.mkdir()
 worker_env=dict(env,SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(frames))
 result=subprocess.run([str(exe),*session,'--geometry-workers',str(workers)],cwd=root,env=worker_env,capture_output=True,text=True,timeout=120)
 log=result.stdout+result.stderr;(output/f'workers-{workers}.txt').write_text(log)
 assert result.returncode==0 and f'geometry_worker_count={workers} range_policy=' in log,log
 assert f'retained_geometry_workers={workers}' in log and 'projected_light_buffer_device_test=passed' in log and 'projected_light_shader_readback=passed' in log and 'light_descriptor_device_test=passed' in log and 'gobo_device_upload_readback=passed' in log and 'session_light_constants_gpu_readback=passed' in log and 'projected_light_graphics_pipeline_device_test=passed' in log and 'projected_light_draws=' in log and 'projected_light_snapshot_source=prepared_frame' in log and 'resources_rebuilt=1 light_count=0' in log and 'resources_rebuilt=1 light_count=256' in log,log
 updates=re.findall(r'projected_light_frame_update=(\d+) buffers_updated=(\d+) submission_pending=(\d+)',log)
 assert updates==[('0','0','0'),('2','0','0'),('0','0','0'),('1','0','0'),('2','0','0')],updates
 counts=re.findall(r'resources_rebuilt=(\d+) light_count=(\d+)',log)
 assert counts==[('0','3'),('1','256'),('1','3'),('1','0'),('1','256')],counts
 assert 'projected_light_shader_readback=passed records=256 float_words=7168 byte_exact=1' in log
 draws=[int(n) for n in re.findall(r'projected_light_draws=(\d+)',log)]
 assert len(draws)==5 and draws[3]==0 and draws[1]>=256 and draws[1]%256==0 and draws[4]==draws[1],draws
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
expected_hashes={
 'vulkan-retained-frame-0.rgba':'d8af74cfdb6c6047240d85d0338a76aba5f04365fb82abf12220953eb4626df8',
 'vulkan-retained-frame-1.rgba':'bfe5e30c7becc02edc93f1f154068d052e6612e1778fbf24de3e69b0ec092ae1',
 'vulkan-retained-frame-2.rgba':'1cdbab9f121cefb12838dd1c28b4d7f78afb7d5735171b8136f11b24f5b33de7'}
assert all(frame_hashes[k]==expected_hashes[k] for k in expected_hashes if 'frame-2' not in k),frame_hashes
assert frame_hashes['vulkan-retained-frame-2.rgba']!=expected_hashes['vulkan-retained-frame-2.rgba'],frame_hashes
reference=(root/'outputs/prepared-light-source-launch-673ef82c/frames-1/vulkan-retained-frame-2.rgba').read_bytes()
reduced=(output/'frames-1/vulkan-retained-frame-2.rgba').read_bytes()
assert all(reduced[i]==reference[i] if i%4==3 else reduced[i]>=reference[i] for i in range(len(reduced)))
assert any(reduced[i]>reference[i] for i in range(len(reduced)) if i%4!=3)
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
 scope='Actual device storage buffer allocation and host-mapped byte verification, plus native frame regression; three-record compute shader field readback bit-identical; actual material/light descriptor pool creation and writes pass; actual installed gobo image upload/copy-readback byte-exact; actual first-shot light constants shader-readback exact; projected light graphics pipeline device-created; actual projected-light draws at gain1 with SDK-derived reciprocal-distance attenuation/far fade; initial light resources read from actual prepared frame; controlled replicated 3/256/3/0/256 light input capacity fixture; 7168 words GPU readback exact and per-light draws submitted; shadow maps and 256 shadowed lights NOT implemented; Source lighting parity unverified.')
(output/'validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
