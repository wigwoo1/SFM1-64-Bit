from pathlib import Path
import os,subprocess,json
baseline=Path('work/retained-shot-sequence-v1/frame-0/manifest.txt').read_text();model=next(line for line in baseline.splitlines() if line.startswith('model-0'))
cases={'multiple-models':baseline+'\n'+model.replace('model-0','model-1',1)+'\n','different-model':baseline.replace('models/anno/mamagen/mamagen.mdl','models/anno/mamagen/other.mdl')}
env=os.environ.copy()
for key in ('BONE_POSE','MODEL_MATRIX','CAMERA_MATRIX','BLEND_TEST'):env.pop('SFM64_DIAGNOSTIC_'+key,None)
env['SFM64_DIAGNOSTIC_FRAME_COUNT']='1';results=[]
for name,manifest in cases.items():
 directory=Path('work')/('vulkan-sequence-reject-'+name);frame=directory/'frame-0';frame.mkdir(parents=True,exist_ok=True);(frame/'manifest.txt').write_text(manifest);env['SFM64_DIAGNOSTIC_FRAME_SEQUENCE']=str(directory)
 result=subprocess.run(['build/session-core/probe_vulkan_graphics_x64.exe','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/models/anno/mamagen/mamagen','D:/SteamLibrary/steamapps/common/SourceFilmmaker/game/usermod/materials/models/anno/mamagen/eyes.vtf'],env=env,capture_output=True,text=True)
 assert result.returncode!=0 and ('sequence supports only one model' in result.stderr or 'sequence model changed' in result.stderr),(name,result.stdout,result.stderr)
 results.append({'case':name,'rejected':True,'error':result.stderr.strip()})
Path('outputs/sfm64-vulkan-sequence-rejection-validation.json').write_text(json.dumps(results,indent=2)+'\n')
print('Vulkan sequence rejects multiple-model and changed-model manifests before frame submission')
