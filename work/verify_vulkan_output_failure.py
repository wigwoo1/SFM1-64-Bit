from pathlib import Path
import subprocess,os,json
env=os.environ.copy()
for key in ('BONE_POSE','MODEL_MATRIX','CAMERA_MATRIX','BLEND_TEST'):env.pop('SFM64_DIAGNOSTIC_'+key,None)
env.update(SFM64_DIAGNOSTIC_FRAME_SEQUENCE='work/retained-shot-sequence-v1',SFM64_DIAGNOSTIC_FRAME_COUNT='3',SFM64_DIAGNOSTIC_ASYNC_OUTPUT='1')
command=['build/session-core/probe_vulkan_graphics_x64.exe','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/anno/mamagen/mamagen','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/materials/models/anno/mamagen/eyes.vtf']
output=Path('work/vulkan-custom-output-v1');output.mkdir(exist_ok=True);env['SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY']=str(output)
valid=subprocess.run(command,env=env,capture_output=True,text=True,timeout=60);assert valid.returncode==0,valid.stderr
for n in range(3):assert (output/f'vulkan-retained-frame-{n}.rgba').read_bytes()==Path('outputs',('mamagen-vulkan-sampled-shot-second-tick-v1.rgba' if n==1 else 'mamagen-vulkan-sampled-shot-v1.rgba')).read_bytes()
bad=Path('work/vulkan-output-failure-v1');(bad/'vulkan-retained-frame-0.rgba').mkdir(parents=True,exist_ok=True);env['SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY']=str(bad)
failed=subprocess.run(command,env=env,capture_output=True,text=True,timeout=60)
assert failed.returncode!=0 and 'frame output write failed' in failed.stderr and 'retained Vulkan sequence passed' not in failed.stdout
Path('outputs/sfm64-vulkan-output-failure-validation.json').write_text(json.dumps({'custom_directory_frames_exact':3,'asynchronous_failure_propagated':True,'success_not_reported':True,'error':failed.stderr.strip()},indent=2)+'\n')
print('custom output directory: 3 frames exact; background output failure propagated through Vulkan teardown')
