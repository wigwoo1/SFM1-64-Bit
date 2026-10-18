from pathlib import Path
import os,subprocess,shutil,json
source=Path('work/retained-shot-sequence-v1');sequence=Path('work/presentation-late-failure-v1')
for frame in (0,1):
 directory=sequence/f'frame-{frame}';directory.mkdir(parents=True,exist_ok=True)
 for name in ('camera.bin','model-0-placement.bin','model-0-pose.bin','manifest.txt'):shutil.copyfile(source/f'frame-{frame}'/name,directory/name)
manifest=sequence/'frame-1/manifest.txt';manifest.write_text(manifest.read_text().replace('models/anno/mamagen/mamagen.mdl','models/anno/mamagen/other.mdl'))
env=os.environ.copy()
for key in ('BONE_POSE','MODEL_MATRIX','CAMERA_MATRIX','BLEND_TEST'):env.pop('SFM64_DIAGNOSTIC_'+key,None)
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL='D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/anno/mamagen/mamagen',SFM64_DIAGNOSTIC_FRAME_SEQUENCE=str(sequence),SFM64_DIAGNOSTIC_FRAME_COUNT='2')
result=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
assert result.returncode!=0 and 'sequence model changed' in result.stderr
assert 'source=model_gpu_image frames=1' in result.stdout
assert 'model_gpu_presentation_passed=1' not in result.stdout
Path('outputs/sfm64-presenter-late-failure-validation.txt').write_text(result.stdout+result.stderr)
Path('outputs/sfm64-presenter-late-failure-summary.json').write_text(json.dumps({'first_model_frame_presented':True,'next_frame_rejected':True,'process_returned_failure':True,'success_not_reported':True,'presentation_semaphores':'per image','error':result.stderr.strip()},indent=2)+'\n')
print('later-frame failure propagates after first presentation; presenter teardown completed')
