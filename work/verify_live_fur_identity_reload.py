from pathlib import Path
import subprocess,os,uuid,json,hashlib
root=Path(__file__).resolve().parent.parent
directory=root/'outputs'/('fur-identity-'+uuid.uuid4().hex);directory.mkdir()
fixture=root/'work/roundtrip-11.dmx';before=fixture.read_bytes()
generated=subprocess.run([str(root/'work/test_retained_fur_scene.exe'),str(directory)],cwd=root,capture_output=True,text=True,timeout=60);assert generated.returncode==0,generated.stdout+generated.stderr
exe=root/'build/session-core/probe_vulkan_win32_surface_x64.exe'
ticks=directory/'ticks.txt';ticks.write_text('870833\n870833\n870833\n')
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=str(fixture),SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_COUNT='3')
results={}
for name,other,fur,prefetch in [('baseline',None,False,0),('fur',None,True,0),('relocated-sync','relocated.dmx',True,0),('relocated-prefetch','relocated.dmx',True,1),('missing-sync','missing.dmx',True,0),('missing-prefetch','missing.dmx',True,1)]:
 folder=directory/name;folder.mkdir();current=env|{'SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY':str(folder),'SFM64_DIAGNOSTIC_FRAME_PREFETCH':str(prefetch)}
 if fur:current['SFM64_DIAGNOSTIC_FUR_TEST']='1'
 if other:
  sequence=folder/'sessions.txt';sequence.write_text('\n'.join(map(str,[fixture,directory/other,fixture]))+'\n');current['SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE']=str(sequence)
 run=subprocess.run([str(exe)],cwd=root,env=current,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;(folder/'log.txt').write_text(log);assert run.returncode==0,log
 results[name]=[(folder/f'vulkan-retained-frame-{i}.rgba').read_bytes() for i in range(3)]
for name in ['relocated-sync','relocated-prefetch']:assert results[name]==results['fur'],name
for name in ['missing-sync','missing-prefetch']:
 assert results[name][0]==results['fur'][0] and results[name][1]==results['baseline'][1] and results[name][2]==results['fur'][2],name
assert results['fur'][1]!=results['baseline'][1] and fixture.read_bytes()==before
report={'relocated_model_gpu_pixels_exact':True,'missing_uuid_base_pixels_exact':True,'returning_uuid_fur_pixels_exact':True,'synchronous_prefetched_match':True,'source_unchanged':True,'output_directory':str(directory),'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'scope':'One installed model/session reload; full SFM/native renderer parity unfinished.'}
(root/'outputs/sfm64-live-fur-identity-reload-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('fur_UUID_reload_GPU_exact=1 missing_skips=1 returning_restores=1 prefetch_modes=2')
