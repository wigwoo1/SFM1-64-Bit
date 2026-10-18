"""Protocol fixtures validate rejection; no actual full-export claim."""
import json
import tempfile
from pathlib import Path
from PIL import Image
from sfm_cycles_conversion_candidate import file_digest
from export_result_acceptance_candidate import accept_export_result,capture_accepted_image
with tempfile.TemporaryDirectory() as temporary:
    root=Path(temporary);backend=root/'backend';(backend/'renders/render').mkdir(parents=True);(backend/'cj/conversion').mkdir(parents=True)
    session=root/'session.dmx';session.write_bytes(b'capture fixture')
    blend=backend/'cj/conversion/session.blend';blend.write_bytes(b'converted fixture')
    png=backend/'renders/render/render.png';Image.new('RGB',(64,64)).save(png)
    request=dict(version=1,request_id='request',editor_revision=7,session=str(session),settings=dict(renderer='cycles',resolution=[64,64],scene='shot',cycles={'samples':8}))
    status=dict(request_id='request',state='complete',job_id='conversion',output_png=str(png))
    prepared=dict(job=dict(run_id='conversion',session_sha256=file_digest(session),output=str(blend)))
    outcome=dict(state='complete',job_id='conversion',conversion=dict(state='complete',job_id='conversion',exit_code=0,result=dict(input_blend=str(blend),fidelity='partial')),
                 render=dict(state='complete',job_id='render',exit_code=0,result=dict(version=1,status='rendered',job_id='render',output_png=str(png),input_sha256=file_digest(blend),resolution=[64,64],scene='shot',frame=1,effective_cycles={'shot':{'samples':8}})))
    child_job=dict(job_id='render',version=1,mode='render',input_blend=str(blend),resolution=[64,64],scene='shot',frame=1,cycles={'samples':8})
    def write():
        for path,value in ((root/'request.json',request),(backend/'status.json',status),(backend/'prepared.json',prepared),(backend/'result.json',outcome)):path.write_text(json.dumps(value))
        (png.parent/'job.json').write_text(json.dumps(child_job))
        (png.parent/'result.json').write_text(json.dumps(outcome['render']['result']))
    outcome['render']['result']['output_sha256']=file_digest(png)
    write();assert accept_export_result(root/'request.json',0,7)['fidelity']=='partial'
    cases=0
    def reject(code=0,revision=7):
        global cases
        try:accept_export_result(root/'request.json',code,revision)
        except (ValueError,KeyError):cases+=1
        else:raise AssertionError('Invalid result accepted')
    reject(revision=8);reject(code=1)
    status['request_id']='other';write();reject();status['request_id']='request'
    status['state']='cancelled';write();reject();status['state']='complete'
    report=outcome['render']['result']
    expected_hash=report.pop('output_sha256');write();reject();report['output_sha256']=expected_hash
    Image.new('RGB',(64,64),'red').save(png);write();reject();Image.new('RGB',(64,64)).save(png)
    report['effective_cycles']['shot']['samples']=9;write();reject();report['effective_cycles']['shot']['samples']=8
    report['scene']='wrong';write();reject();report['scene']='shot'
    report['output_png']=str(root/'outside.png');write();reject();report['output_png']=str(png)
    outcome['render']['exit_code']=False;write();reject();outcome['render']['exit_code']=0
    child_job['cycles']['samples']=4;write();reject();child_job['cycles']['samples']=8
    write();different=dict(report,status='failed');(png.parent/'result.json').write_text(json.dumps(different));reject()
    write();report['effective_cycles']={'other':{'samples':8}};write();reject();report['effective_cycles']={'shot':{'samples':8}}
    write();blend.write_bytes(b'changed');reject();blend.write_bytes(b'converted fixture')
    session.write_bytes(b'changed');reject();session.write_bytes(b'capture fixture')
    Image.new('RGB',(65,64)).save(png);reject();Image.new('RGB',(64,64)).save(png)
    data=png.read_bytes();png.write_bytes(data+b'trailing');reject()
    png.write_bytes(data);capture=capture_accepted_image(root/'request.json',0,7)
    assert Path(capture['presentation_png']).read_bytes()==data
    fixed=backend/'accepted-deadbeef'
    fixed_capture=capture_accepted_image(root/'request.json',0,7,fixed)
    assert Path(fixed_capture['presentation_png']).read_bytes()==data
    try:capture_accepted_image(root/'request.json',0,7,fixed)
    except FileExistsError:pass
    else:raise AssertionError('Presentation folder reused')
    try:capture_accepted_image(root/'request.json',0,7,root/'accepted-12345678')
    except ValueError:pass
    else:raise AssertionError('Outside presentation folder accepted')
    Image.new('RGB',(64,64),'red').save(png)
    assert Path(capture['presentation_png']).read_bytes()==data
    print('PASS accepted protocol fixture and',cases,'rejections; full actual native export remains separate')
