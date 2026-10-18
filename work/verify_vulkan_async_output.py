from pathlib import Path
import os,subprocess,json
env=os.environ.copy()
for key in ('BONE_POSE','MODEL_MATRIX','CAMERA_MATRIX','BLEND_TEST'):env.pop('SFM64_DIAGNOSTIC_'+key,None)
env['SFM64_DIAGNOSTIC_FRAME_SEQUENCE']='work/retained-shot-fence-reuse-v1';env['SFM64_DIAGNOSTIC_FRAME_COUNT']='9';env['SFM64_DIAGNOSTIC_FRAME_PREFETCH']='1'
command=['build/session-core/probe_vulkan_graphics_x64.exe','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/anno/mamagen/mamagen','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/materials/models/anno/mamagen/eyes.vtf']
baseline=None;checks=[]
for mode,writes in (('0',0),('1',9)):
 env['SFM64_DIAGNOSTIC_ASYNC_OUTPUT']=mode;result=subprocess.run(command,env=env,capture_output=True,text=True,timeout=60)
 Path(f'outputs/sfm64-vulkan-async-output-mode-{mode}.txt').write_text(result.stdout+result.stderr)
 assert result.returncode==0,(result.stdout,result.stderr)
 assert f'async_output_writes={writes} pending_output_bound=1' in result.stdout
 frames=[Path(f'outputs/vulkan-retained-frame-{n}.rgba').read_bytes() for n in range(9)]
 if baseline is None:baseline=frames
 else:assert baseline==frames,'asynchronous output differs'
 checks.append({'asynchronous_output':bool(int(mode)),'frames':9,'background_writes':writes})
Path('outputs/sfm64-vulkan-async-output-validation.json').write_text(json.dumps({'modes':checks,'pixel_exact':True,'pending_frame_bound':1,'flush_before_success':True,'native_render_parity':'unverified'},indent=2)+'\n')
print('synchronous/asynchronous output: 9 frames byte-exact and final writes completed before success')
