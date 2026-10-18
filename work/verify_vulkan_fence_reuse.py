from pathlib import Path
import os,subprocess,shutil,json
source=Path('work/retained-shot-sequence-v1');sequence=Path('work/retained-shot-fence-reuse-v1')
for frame in range(9):
 target=sequence/f'frame-{frame}';target.mkdir(parents=True,exist_ok=True)
 for name in ('camera.bin','model-0-placement.bin','model-0-pose.bin','manifest.txt'):shutil.copyfile(source/f'frame-{frame%3}'/name,target/name)
env=os.environ.copy()
for key in ('BONE_POSE','MODEL_MATRIX','CAMERA_MATRIX','BLEND_TEST'):env.pop('SFM64_DIAGNOSTIC_'+key,None)
env['SFM64_DIAGNOSTIC_FRAME_SEQUENCE']=str(sequence);env['SFM64_DIAGNOSTIC_FRAME_COUNT']='9'
command=['build/session-core/probe_vulkan_graphics_x64.exe','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/anno/mamagen/mamagen','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/materials/models/anno/mamagen/eyes.vtf']
result=subprocess.run(command,env=env,capture_output=True,text=True,timeout=60)
Path('outputs/sfm64-vulkan-fence-reuse-validation.txt').write_text(result.stdout+result.stderr)
assert result.returncode==0,(result.stdout,result.stderr)
assert 'gpu_wait=submission_fence fence_waits=9 queue_idle_waits=0' in result.stdout
refs=('mamagen-vulkan-sampled-shot-v1.rgba','mamagen-vulkan-sampled-shot-second-tick-v1.rgba','mamagen-vulkan-sampled-shot-v1.rgba')
for frame in range(9):assert Path(f'outputs/vulkan-retained-frame-{frame}.rgba').read_bytes()==Path('outputs',refs[frame%3]).read_bytes(),frame
# Reject a model change after one completed GPU submission, exercising teardown.
manifest=sequence/'frame-1/manifest.txt';saved=manifest.read_text()
try:
 manifest.write_text(saved.replace('models/anno/mamagen/mamagen.mdl','models/anno/mamagen/other.mdl'))
 env['SFM64_DIAGNOSTIC_FRAME_COUNT']='2';failed=subprocess.run(command,env=env,capture_output=True,text=True,timeout=60)
 assert failed.returncode!=0 and 'sequence model changed' in failed.stderr and 'RGBA frame=0' in failed.stdout
finally:manifest.write_text(saved)
Path('outputs/sfm64-vulkan-fence-reuse-summary.json').write_text(json.dumps({'frames':9,'pixel_exact_frames':9,'fence_waits':9,'normal_queue_idle_waits':0,'late_frame_rejection':True,'native_render_parity':'unverified'},indent=2)+'\n')
print('9 fence submissions pixel-exact; later-frame rejection exits cleanly after completed submission')
