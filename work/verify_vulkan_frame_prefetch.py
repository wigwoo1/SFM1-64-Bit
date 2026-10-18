from pathlib import Path
import os,subprocess,json,hashlib
env=os.environ.copy()
for key in ('BONE_POSE','MODEL_MATRIX','CAMERA_MATRIX','BLEND_TEST'):env.pop('SFM64_DIAGNOSTIC_'+key,None)
env['SFM64_DIAGNOSTIC_FRAME_SEQUENCE']='work/retained-shot-fence-reuse-v1';env['SFM64_DIAGNOSTIC_FRAME_COUNT']='9'
command=['build/session-core/probe_vulkan_graphics_x64.exe','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/anno/mamagen/mamagen','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/materials/models/anno/mamagen/eyes.vtf']
checks=[];serial=None
for mode,tasks in (('0',0),('1',8)):
 env['SFM64_DIAGNOSTIC_FRAME_PREFETCH']=mode
 result=subprocess.run(command,env=env,capture_output=True,text=True,timeout=60)
 Path(f'outputs/sfm64-vulkan-frame-prefetch-mode-{mode}.txt').write_text(result.stdout+result.stderr)
 assert result.returncode==0,(result.stdout,result.stderr)
 assert f'preparation_tasks={tasks} max_prepared_ahead=1' in result.stdout
 assert 'fence_waits=9 queue_idle_waits=0' in result.stdout
 frames=[Path(f'outputs/vulkan-retained-frame-{n}.rgba').read_bytes() for n in range(9)]
 if serial is None:serial=frames
 else:assert frames==serial,'asynchronous preparation changed output'
 checks.append({'prefetch':mode=='1','preparation_tasks':tasks,'frames':9,'sha256':[hashlib.sha256(raw).hexdigest() for raw in frames]})
Path('outputs/sfm64-vulkan-frame-prefetch-validation.json').write_text(json.dumps({'modes':checks,'pixel_exact':True,'queued_preparation_bound':1,'launch_order':'after submit before fence wait','performance_gain':'unmeasured','native_render_parity':'unverified'},indent=2)+'\n')
print('prefetch enabled/disabled: all 9 frames byte-exact; 8 asynchronous preparation tasks versus 0')
