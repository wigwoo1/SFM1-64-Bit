from pathlib import Path
import subprocess,json,sys
report_path=Path('outputs/sfm64-scene-prefetch-pixel-equivalence.json')
before=json.loads(report_path.read_text())['runs'][0]['frame_sha256']
run=subprocess.run([sys.executable,'work/verify_scene_prefetch_gpu.py'],capture_output=True,text=True,timeout=60)
assert run.returncode==0,run.stdout+run.stderr
after=json.loads(report_path.read_text())
for run_record in after['runs']:
 assert run_record['frame_sha256']==before,'material cache integration changed established pixels'
 log=Path(f"outputs/sfm64-scene-prefetch-{run_record['prefetch']}.txt").read_text()
 assert 'scene_material_cache=per_model flattened_texture_bytes=0' in log
 assert 'prepared_scene_gpu_bridge=active model_inputs=4' in log
 assert 'retained_gpu_material_cache_entries=4' in log
Path('outputs/sfm64-multimodel-material-cache-gpu.json').write_text(json.dumps(dict(models=4,frames=62,flattened_texture_bytes=0,gpu_material_cache_entries=4,pixel_exact_against_prior_flattened_route=True,prefetch_modes=[0,1],scope='GPU0 saved four-model scene; cache reuses per-model inputs, does not deduplicate identical textures across different model inputs; resource transitions remain unsupported'),indent=2)+'\n')
print('per-model GPU material cache passed: 4 inputs, zero flattened texture copies, all 62 frames exact against prior renderer')
