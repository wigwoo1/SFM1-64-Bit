from pathlib import Path
import subprocess,os,uuid,json,hashlib
root=Path(__file__).resolve().parent.parent
directory=root/'outputs'/('fur-coverage-'+uuid.uuid4().hex);directory.mkdir()
exe=root/'build/session-core/probe_vulkan_win32_surface_x64.exe'
fixture=root/'work/short-timeline-session.dmx';before=fixture.read_bytes()
base=[str(exe),'--session',str(fixture),'--content-directory',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test','--fur','2552','500','2','0.05','--fur-color','255','200','100','--fur-workers','4','--fur-seed','42']
environment={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
environment.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1')
captures={}
for name,args in [('solid',[]),('coverage',['--fur-coverage'])]:
 folder=directory/name;folder.mkdir();env=environment|{'SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY':str(folder)}
 result=subprocess.run(base+args,cwd=root,env=env,capture_output=True,text=True,timeout=60)
 log=result.stdout+result.stderr;(folder/'log.txt').write_text(log);assert result.returncode==0,log
 captures[name]={p.name:p.read_bytes() for p in folder.glob('*.rgba')};assert captures[name]
assert captures['solid'].keys()==captures['coverage'].keys()
changed={name:sum(a!=b for a,b in zip(captures['solid'][name],captures['coverage'][name])) for name in captures['solid']};assert all(changed.values()),changed
previous=json.loads((root/'outputs/sfm64-live-fur-launch-validation.json').read_text());old={p.name:p.read_bytes() for p in Path(previous['output_directory']).glob('*.rgba')}
assert old==captures['solid'],'solid mode changed from pre-cutoff reference'
assert fixture.read_bytes()==before
report={'coverage_changed_bytes_by_frame':changed,'solid_mode_matches_pre_cutoff_reference':True,'input_unchanged':True,'output_directory':str(directory),'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'limits':'Unlit alpha cutout strands. Grooming, lighting, antialiasing and native renderer parity unfinished.'}
(root/'outputs/sfm64-live-fur-coverage-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('fur_coverage_gpu_changes='+str(changed)+' solid_reference_exact=1')
