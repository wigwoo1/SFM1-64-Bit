from pathlib import Path
import subprocess,os,json,uuid,hashlib
root=Path(__file__).resolve().parent.parent
directory=root/'outputs'/('live-fur-gpu-'+uuid.uuid4().hex)
directory.mkdir()
fixture=root/'work/short-timeline-session.dmx'
before=fixture.read_bytes()
exe=root/'build/session-core/probe_vulkan_win32_surface_x64.exe'
captures=[]
for label,enabled,workers in [('baseline',False,0),('fur',True,0),('fur-workers4',True,4)]:
 output=directory/label;output.mkdir()
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(output))
 if enabled:
  env['SFM64_DIAGNOSTIC_FUR_TEST']='1'
  env['SFM64_DIAGNOSTIC_FUR_WORKERS']=str(workers)
 result=subprocess.run([str(exe),'--session',str(fixture),'--content-directory',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test'],cwd=root,env=env,capture_output=True,text=True,timeout=60)
 log=result.stdout+result.stderr;(output/'run.txt').write_text(log)
 assert result.returncode==0,log
 if enabled:
  assert 'fur_scene_strands=1000 vertices=4000 material=solid_fur' in log,log
  assert 'retained_gpu_material_cache_entries=2' in log,log
  assert 'scene_fur_workers='+str(workers) in log,log
 frames={p.name:p.read_bytes() for p in output.glob('*.rgba')}
 assert frames,'No GPU captures'
 captures.append(frames)
common=sorted(set(captures[0])&set(captures[1]));assert common,'No matching captured frames'
changes=[]
for name in common:
 a,b=captures[0][name],captures[1][name];assert len(a)==len(b)
 pixels=sum(a[i:i+4]!=b[i:i+4] for i in range(0,len(a),4));changes.append({'frame':name,'changed_pixels':pixels})
assert any(item['changed_pixels'] for item in changes),'Fur produced no captured pixel changes'
assert captures[1]==captures[2],'Parallel fur GPU captures differ from serial'
for i,workers in enumerate(['-1','65','4junk']):
 output=directory/('invalid-workers-'+str(i));output.mkdir()
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_FUR_TEST='1',SFM64_DIAGNOSTIC_FUR_WORKERS=workers,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(output))
 result=subprocess.run([str(exe),'--session',str(fixture),'--content-directory',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test'],cwd=root,env=env,capture_output=True,text=True,timeout=60)
 log=result.stdout+result.stderr;(output/'run.txt').write_text(log)
 assert result.returncode!=0 and 'fur worker option must be 0..64' in log,log
 assert not list(output.glob('*.rgba')),'Invalid worker setting rendered frames'
assert fixture.read_bytes()==before
report={'frames':changes,'strands':1000,'serial_workers4_gpu_pixels_exact':True,'invalid_worker_rejections':3,'output_directory':str(directory),'input_unchanged':True,'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'limits':'Diagnostic attachment on model2552 with an owned solid-color opaque fur material. Pixel differences prove GPU drawing, not final fur shading, transparency, grooming, persistence or native renderer parity.'}
(root/'outputs/sfm64-live-fur-gpu-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('live_fur_gpu_pixel_changes='+str(changes))
