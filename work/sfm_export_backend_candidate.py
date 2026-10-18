"""Private export process entry point for native captured-session requests."""
import argparse
import json
import os
import time
import uuid
from pathlib import Path
from sfm_cycles_conversion_candidate import prepare_conversion
from sfm_cycles_bridge_candidate import SfmCyclesBridgeCandidate
from cycles_settings_candidate import validate_settings

def write_state(path, value):
    temporary=path.with_suffix('.pending')
    with temporary.open('w',encoding='utf-8') as stream:
        json.dump(value,stream,allow_nan=False)
        stream.flush();os.fsync(stream.fileno())
    os.replace(temporary,path)

def write_export_status(path,value):
    labels={'preparing':'Preparing export','converting':'Converting Source session',
            'rendering':'Rendering with Cycles','cancelling':'Stopping export',
            'cancelled':'Export cancelled','complete':'Render finished','failed':'Export failed'}
    lines=[labels[value['state']]]
    progress=value.get('progress')
    if progress and progress.get('message'):lines.append(progress['message'])
    if value.get('error'):lines.append(value['error'])
    if value.get('fidelity')=='partial':lines.append('Conversion has fidelity warnings; inspect the conversion report.')
    text_path=path.with_suffix('.txt');temporary=text_path.with_suffix('.text-pending')
    temporary.write_bytes('\r\n'.join(lines).encode('utf-8'))
    os.replace(temporary,text_path)
    write_state(path,value)

def run(request_path):
    request_path=Path(request_path).resolve()
    request=json.loads(request_path.read_text(encoding='utf-8'))
    if request.get('version')!=1:raise ValueError('Unsupported export request version')
    identifier=str(uuid.UUID(request['request_id']))
    if identifier!=request['request_id']:raise ValueError('Canonical export request identity required')
    settings=request['settings']
    if type(settings) is not dict or settings.get('renderer')!='cycles':
        raise ValueError('Only Cycles export dispatch is currently implemented')
    # Validate against the last installed-version inspection before conversion;
    # the render worker independently discovers and validates its live schema.
    schema=json.loads(Path(request['cycles_schema']).read_text())['cycles_schema']
    validate_settings(schema,settings.get('cycles',{}))
    session=Path(request['session']).resolve()
    if not session.is_relative_to(request_path.parent):
        raise ValueError('Export requires a captured session inside the request folder')
    folder=request_path.parent/'backend'
    folder.mkdir() # Never reuse another request's state or cancellation marker.
    state_path=folder/'status.json'
    bridge=None
    try:
        write_export_status(state_path,dict(request_id=identifier,state='preparing'))
        prepared=prepare_conversion(session,request['sfm'],request['converter_snapshot'],
                                    folder/'cj',request['blender'])
        write_state(folder/'prepared.json',prepared)
        bridge=SfmCyclesBridgeCandidate(request['blender'],folder/'cj',folder/'renders')
        job=bridge.start(prepared,settings,current_session=session,cycles_schema=schema)
        last=None
        while True:
            if (folder/'cancel').exists() or (request_path.parent/'cancel').exists():bridge.cancel(job)
            outcome=bridge.poll(job)
            state=outcome['state']
            compact=dict(request_id=identifier,job_id=job,state=state)
            for key in ('progress','error','pid'):
                if key in outcome:compact[key]=outcome[key]
            if state=='complete':
                compact.update(output_png=outcome['render']['result']['output_png'],
                    fidelity=outcome['conversion']['result']['fidelity'])
            if state not in ('converting','rendering','cancelling'):
                write_state(folder/'result.json',outcome)
                write_export_status(state_path,compact)
                return 0 if state in ('complete','cancelled') else 1
            if compact!=last:write_export_status(state_path,compact);last=compact
            time.sleep(.1)
    except Exception as error:
        if bridge:bridge.close();bridge=None
        write_export_status(state_path,dict(request_id=identifier,state='failed',error=str(error)))
        raise
    finally:
        if bridge:bridge.close()

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('request')
    args=parser.parse_args()
    raise SystemExit(run(args.request))
