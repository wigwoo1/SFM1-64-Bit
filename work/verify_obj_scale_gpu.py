from pathlib import Path
import runpy,subprocess,json
state=runpy.run_path('work/verify_obj_gpu_import.py');out=state['out'];baseline=state['frames']['baseline'];original=state['frames']['imported'];reports={}
for factor in ['1','0.5']:
 folder=out/('scale-'+factor);folder.mkdir(exist_ok=True);env=state['env'].copy();env['SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY']=str(folder.resolve());run=subprocess.run(state['args']+['--obj-scale',factor],env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;(out/('scale-'+factor+'.txt')).write_text(log);assert run.returncode==0,log;pixels=(folder/'vulkan-retained-frame-0.rgba').read_bytes()
 if factor=='1':assert pixels==original,'identity scale changes pixels'
 else:
  outside=0;white=0;changed=0
  for y in range(512):
   for x in range(512):
    i=(y*512+x)*4
    if pixels[i:i+4]!=baseline[i:i+4]:
     changed+=1
     if not (178<=x<=334 and 178<=y<=334):outside+=1
    if 181<=x<=330 and 181<=y<=330 and pixels[i:i+3]==b'\xff\xff\xff':white+=1
  assert outside==0 and white>22000 and 10000<changed<35000,(outside,white,changed)
  reports=dict(half_scale_changed_pixels=changed,outside_expected_bounds=outside,inner_white_pixels=white)
invalid=subprocess.run(state['args']+['--obj-scale','0'],env=env,capture_output=True,text=True,timeout=30);assert invalid.returncode!=0 and 'OBJ scale must be finite and positive' in invalid.stderr
reports.update(actual_gpu=True,identity_scale_pixels_exact=True,zero_scale_rejected=True,pivot='imported bounds center',limits='Uniform positive scale via launch argument. In-program scale controls, persistence, FBX rigs and nonuniform scaling unfinished.')
(out/'scale-report.json').write_text(json.dumps(reports,indent=2)+'\n');print('obj_gpu_uniform_scale=passed '+str(reports))
