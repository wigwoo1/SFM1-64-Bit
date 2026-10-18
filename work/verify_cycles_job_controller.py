import json,time,hashlib
from pathlib import Path
from cycles_job_controller_candidate import CyclesJobControllerCandidate
root=Path('outputs/cycles-controller');controller=CyclesJobControllerCandidate(r'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe',root)
def await_job(identifier):
 deadline=time.monotonic()+120;polls=0
 while time.monotonic()<deadline:
  result=controller.poll(identifier);polls+=1
  if result['state'] not in ('running','cancelling'):return result,polls
  time.sleep(.2)
 controller.cancel(identifier);raise AssertionError('Job exceeded watchdog')
job=json.loads(Path('outputs/cycles-worker/session-render-job.json').read_text());source=Path(job['input_blend']);before=hashlib.sha256(source.read_bytes()).hexdigest()
dimensions=job['resolution'][:];valid=controller.start(job);job['resolution'][0]=512;assert controller.poll(valid)['state']=='running';result,polls=await_job(valid);assert result['state']=='complete',result;assert polls>1;job['resolution']=dimensions
invalid=controller.start(dict(job,cycles={'samples':0}));rejected,_=await_job(invalid);assert rejected['state']=='failed' and 'supported range' in rejected['error'],rejected
full_job=json.loads(Path('outputs/cycles-worker/full-session-render-job.json').read_text())
with Path(full_job['input_blend']).open('rb') as stream:full_before=hashlib.file_digest(stream,'sha256').hexdigest()
cancelled=controller.start(full_job);deadline=time.monotonic()+120
while time.monotonic()<deadline:
 assert controller.poll(cancelled)['state']=='running'
 log=controller.jobs[cancelled]['folder']/'worker.log'
 if '"event": "render_started"' in log.read_text(errors='replace'):break
 time.sleep(.2)
else:controller.close();raise AssertionError('Active render did not start')
start=time.monotonic();assert controller.cancel(cancelled);cancel_signal_seconds=time.monotonic()-start;assert cancel_signal_seconds<1
state,_=await_job(cancelled);assert state['state']=='cancelled' and controller.jobs[cancelled]['process'].poll() is not None
wrong=controller.start(dict(job,mode='inspect'));controller.jobs[wrong]['process'].wait(timeout=120);path=controller.jobs[wrong]['folder']/'result.json';data=json.loads(path.read_text());data['job_id']='other-job';path.write_text(json.dumps(data));assert 'identity mismatch' in controller.poll(wrong)['error']
damaged=controller.start(job);controller.jobs[damaged]['process'].wait(timeout=120);png=controller.jobs[damaged]['folder']/'render.png';data=png.read_bytes();png.write_bytes(data[:-1]);assert controller.poll(damaged)['state']=='failed' 
assert controller.poll(valid)['state']=='complete' and not controller.cancel(valid)
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
with Path(full_job['input_blend']).open('rb') as stream:assert hashlib.file_digest(stream,'sha256').hexdigest()==full_before
controller.close();report=dict(async_polls=polls,validated_render=True,input_parameters_snapshot=True,invalid_settings_failed=True,cancelled_private_process=True,cancel_during_active_render=True,cancel_signal_seconds=cancel_signal_seconds,result_identity_checked=True,truncated_png_rejected=True,completed_result_retained=True,input_unchanged=True,limits='Controller/backend foundation; native SFM UI and automatic DMX conversion unfinished.')
root.mkdir(exist_ok=True);(root/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('cycles_job_controller=passed '+str(report))
