from pathlib import Path
import subprocess,os,uuid,json,hashlib
root=Path(__file__).resolve().parent.parent
directory=root/'outputs'/('fur-settings-'+uuid.uuid4().hex);directory.mkdir()
exe=root/'build/session-core/probe_vulkan_win32_surface_x64.exe'
fixture=root/'work/short-timeline-session.dmx';before=fixture.read_bytes()
base=[str(exe),'--session',str(fixture),'--content-directory',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test']
environment={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
environment.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1')
settings=directory/'model.sfm64fur'
captures=[]
for name,args in [('save',['--fur','2552','500','2','0.05','--fur-coverage','--fur-color','255','200','100','--fur-workers','4','--fur-seed','18446744073709551615','--save-fur-settings',str(settings)]),('reload',['--fur-settings',str(settings)])]:
 folder=directory/name;folder.mkdir();env=environment|{'SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY':str(folder)}
 result=subprocess.run(base+args,cwd=root,env=env,capture_output=True,text=True,timeout=60)
 log=result.stdout+result.stderr;(folder/'log.txt').write_text(log)
 assert result.returncode==0 and 'fur_scene_strands=500 vertices=2000 material=solid_fur' in log and 'scene_fur_workers=4' in log,log
 frames={p.name:p.read_bytes() for p in folder.glob('*.rgba')};assert frames,'no captures';captures.append(frames)
assert captures[0]==captures[1],'saved/reloaded GPU pixels differ'
saved=settings.read_bytes()
invalid=[['--fur-settings',str(settings),'--fur','2552','500','2','.05'],['--fur-settings',str(settings),'--fur-color','1','2','3'],['--fur-settings',str(settings),'--fur-workers','4'],['--save-fur-settings',str(directory/'orphan.sfm64fur')],['--fur-settings',str(settings),'--save-fur-settings',str(settings)]]
for i,args in enumerate(invalid):
 result=subprocess.run(base+args,cwd=root,env=environment,capture_output=True,text=True,timeout=60)
 assert result.returncode!=0,(args,result.stdout,result.stderr)
 (directory/f'invalid-{i}.txt').write_text(result.stdout+result.stderr)
assert settings.read_bytes()==saved and fixture.read_bytes()==before
report={'saved_reloaded_gpu_pixels_exact':True,'gpu_frame_count':len(captures[0]),'uint64_max_seed':True,'invalid_launch_rejections':len(invalid),'overwrite_preserved_file':True,'input_unchanged':True,'output_directory':str(directory),'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'limits':'Companion configuration for prototype opaque ribbon fur; grooming, editor controls and finished shading remain unfinished.'}
(root/'outputs/sfm64-live-fur-settings-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('fur_settings_gpu_roundtrip_exact=1 invalid_launch_rejections='+str(len(invalid)))
