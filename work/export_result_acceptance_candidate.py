"""Revalidate a terminal private export before publishing its image."""
import json
import math
from pathlib import Path
from PIL import Image
from sfm_cycles_conversion_candidate import file_digest
from cycles_job_controller_candidate import verify_png_structure_candidate

def accept_export_result(request_path,observed_exit_code,current_revision):
    if type(observed_exit_code) is not int or observed_exit_code!=0:
        raise ValueError('Export backend did not exit successfully')
    path=Path(request_path).resolve();request=json.loads(path.read_text())
    if request.get('version')!=1:raise ValueError('Unsupported export request version')
    if type(current_revision) is not int or current_revision<0:
        raise ValueError('Current editor revision required')
    revision=request.get('editor_revision')
    if type(revision) is not int or revision!=current_revision:
        raise ValueError('Export belongs to an earlier or unknown editor revision')
    backend=path.parent/'backend'
    status=json.loads((backend/'status.json').read_text())
    outcome=json.loads((backend/'result.json').read_text())
    prepared=json.loads((backend/'prepared.json').read_text())
    if status.get('request_id')!=request['request_id'] or status.get('state')!='complete' or outcome.get('state')!='complete':
        raise ValueError('Export result identity or terminal state mismatch')
    conversion=outcome['conversion'];render=outcome['render'];report=render['result']
    identifier=prepared['job']['run_id']
    if status.get('job_id')!=identifier or outcome.get('job_id')!=identifier or conversion.get('job_id')!=identifier:
        raise ValueError('Conversion job identity mismatch')
    if (conversion.get('state')!='complete' or type(conversion.get('exit_code')) is not int or conversion.get('exit_code')!=0 or
        render.get('state')!='complete' or type(render.get('exit_code')) is not int or render.get('exit_code')!=0):
        raise ValueError('Export child did not finish successfully')
    render_id=render['job_id']
    if report.get('job_id')!=render_id or report.get('version')!=1 or report.get('status')!='rendered':
        raise ValueError('Render result identity mismatch')
    output=(backend/'renders'/render_id/'render.png').resolve()
    if not output.is_relative_to((backend/'renders').resolve()) or Path(report['output_png']).resolve()!=output or Path(status['output_png']).resolve()!=output:
        raise ValueError('Render output outside owned job')
    session=(path.parent/'session.dmx').resolve()
    if Path(request['session']).resolve()!=session or file_digest(session)!=prepared['job']['session_sha256']:
        raise ValueError('Captured export session changed')
    blend=Path(prepared['job']['output']).resolve()
    if not blend.is_relative_to((backend/'cj').resolve()) or Path(conversion['result']['input_blend']).resolve()!=blend:
        raise ValueError('Converted input outside owned job')
    if file_digest(blend)!=report['input_sha256']:raise ValueError('Converted input changed after rendering')
    dimensions=request['settings']['resolution']
    child_folder=output.parent
    child_job=json.loads((child_folder/'job.json').read_text())
    child_result=json.loads((child_folder/'result.json').read_text())
    if child_result!=report:raise ValueError('Render result differs from terminal child report')
    output_digest=file_digest(output)
    if report.get('output_sha256')!=output_digest:
        raise ValueError('Render image changed after worker completion')
    if (child_job.get('job_id')!=render_id or child_job.get('version')!=1 or child_job.get('mode')!='render' or
        Path(child_job['input_blend']).resolve()!=blend or child_job.get('resolution')!=dimensions or
        child_job.get('scene')!=report.get('scene') or child_job.get('frame')!=report.get('frame') or
        child_job.get('cycles',{})!=request['settings'].get('cycles',{})):
        raise ValueError('Render job differs from captured request')
    if request['settings'].get('renderer')!='cycles':raise ValueError('Wrong export renderer')
    if 'scene' in request['settings'] and report.get('scene')!=request['settings']['scene']:
        raise ValueError('Rendered scene differs from requested scene')
    if 'frame' in request['settings'] and report.get('frame')!=request['settings']['frame']:
        raise ValueError('Rendered frame differs from requested frame')
    for settings in report['effective_cycles'].values():
        for name,expected in request['settings'].get('cycles',{}).items():
            actual=settings.get(name)
            equal=(type(actual) in (int,float) and math.isfinite(actual) and math.isclose(actual,expected,rel_tol=1e-6,abs_tol=1e-9)) if type(expected) is float else type(actual) is type(expected) and actual==expected
            if not equal:raise ValueError('Effective render setting mismatch: '+name)
    if report.get('scene') not in report['effective_cycles']:raise ValueError('Missing rendered scene effective settings')
    if report['resolution']!=dimensions:raise ValueError('Render settings dimensions mismatch')
    verify_png_structure_candidate(output)
    with Image.open(output) as image:
        image.load()
        if list(image.size)!=dimensions:raise ValueError('Export image dimensions mismatch')
    return dict(request_id=request['request_id'],editor_revision=revision,output_png=str(output),
                output_sha256=output_digest,resolution=dimensions,
                fidelity=conversion['result']['fidelity'])

def capture_accepted_image(request_path,observed_exit_code,current_revision,presentation_folder=None):
    """Create a new immutable presentation copy after terminal validation."""
    import uuid
    import shutil
    accepted=accept_export_result(request_path,observed_exit_code,current_revision)
    backend=Path(request_path).resolve().parent/'backend'
    folder=Path(presentation_folder).resolve() if presentation_folder else backend/('accepted-'+uuid.uuid4().hex[:8])
    import re
    if folder.parent!=backend or not re.fullmatch(r'accepted-[0-9a-f]{8}',folder.name):
        raise ValueError('Presentation folder must be a fresh owned acceptance folder')
    folder.mkdir()
    output=folder/'render.png'
    try:
        shutil.copyfile(accepted['output_png'],output)
        if file_digest(output)!=accepted['output_sha256']:
            raise ValueError('Render image changed while capturing accepted result')
        accepted=dict(accepted,presentation_png=str(output),status='accepted')
        (folder/'accepted.json').write_text(json.dumps(accepted,indent=2),encoding='utf-8')
        return accepted
    except Exception:
        output.unlink(missing_ok=True)
        raise

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser()
    parser.add_argument('request');parser.add_argument('--observed-exit-code',type=int,required=True)
    parser.add_argument('--current-revision',type=int,required=True)
    parser.add_argument('--presentation-folder')
    arguments=parser.parse_args()
    print(json.dumps(capture_accepted_image(arguments.request,arguments.observed_exit_code,arguments.current_revision,arguments.presentation_folder)))
