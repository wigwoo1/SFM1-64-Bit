import json,subprocess,os,hashlib
from pathlib import Path
out=Path('outputs/cycles-worker');job=json.loads((out/'session-render-job.json').read_text());source=Path(job['input_blend']);before=hashlib.sha256(source.read_bytes()).hexdigest();reports={}
env=os.environ.copy();env['TEMP']=env['TMP']=str(Path('work').resolve())
for case,settings in [('unknown',{'not_a_cycles_setting':4}),('range',{'samples':0}),('boolean_type',{'use_denoising':1})]:
 current=job.copy();current.update(cycles=settings,output_png=str((out/(case+'.png')).resolve()),result=str((out/(case+'-result.json')).resolve()));path=out/(case+'-job.json');path.write_text(json.dumps(current,indent=2));args=[r'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe','--background','--disable-autoexec','--threads','1','--python-exit-code','1','--python','work/cycles_render_worker_candidate.py','--',str(path)]
 run=subprocess.run(args,env=env,capture_output=True,text=True,timeout=120,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));(out/(case+'.log')).write_text(run.stdout+run.stderr);result=json.loads(Path(current['result']).read_text());assert run.returncode!=0 and result['status']=='failed' and not Path(current['output_png']).exists(),result;reports[case]=result['error']
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
(out/'validation-report.json').write_text(json.dumps(dict(rejected=reports,input_unchanged=True),indent=2)+'\n');print('cycles_worker_invalid_settings=passed '+str(reports))
