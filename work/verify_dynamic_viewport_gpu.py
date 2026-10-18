from pathlib import Path
import subprocess,sys,json
checks=[('movie','outputs/sfm64-saved-movie-output-gpu.json','work/verify_saved_movie_output_gpu.py'),('single-model','outputs/sfm64-material-overrides-prefetch-pixel-equivalence.json','work/verify_material_override_prefetch_pixels.py')]
records=[]
for name,path,script in checks:
 report=Path(path);before=json.loads(report.read_text())
 before_hashes=[r['frame_sha256'] for r in before['runs']]
 run=subprocess.run([sys.executable,script],capture_output=True,text=True,timeout=60)
 assert run.returncode==0,run.stdout+run.stderr
 after=json.loads(report.read_text());after_hashes=[r['frame_sha256'] for r in after['runs']]
 assert before_hashes==after_hashes,f'dynamic viewport changed {name} pixels'
 records.append(dict(route=name,pixel_exact_against_static_viewport=True,runs=len(after_hashes),frames_per_run=len(after_hashes[0])))
Path('outputs/sfm64-dynamic-viewport-gpu.json').write_text(json.dumps(dict(viewport_and_scissor_dynamic=True,runs=records,scope='GPU0 existing 4K and 512x512 workloads; in-flight output extent changes still require target lifecycle integration'),indent=2)+'\n')
print('dynamic viewport/scissor GPU checks passed: 4K movie and 61-frame 512x512 routes exact against static pipeline captures')
