import json
import shutil
import subprocess
import sys
import time
import uuid
from pathlib import Path
from sfm_cycles_conversion_candidate import file_digest
root=Path(__file__).resolve().parent.parent
identifier=str(uuid.uuid4());folder=root/'work/ex'/identifier[:8];folder.mkdir(parents=True)
source=root/'outputs/obj-gpu-import/session-embedded-obj/resaved-368c7db4-b1e7-434c-8e35-a2c5cff520cb.dmx'
digest=file_digest(source);captured=folder/'session.dmx';shutil.copyfile(source,captured)
pin=json.loads((root/'outputs/cycles-worker/converter-snapshot.json').read_text())
request=dict(version=1,request_id=identifier,session=str(captured),sfm=r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game',
    converter_snapshot=pin['snapshot'],blender=r'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe',
    cycles_schema=str(root/'outputs/cycles-worker/dynamic-enum-result.json'),
    settings=dict(renderer='cycles',resolution=[1920,1080],cycles=dict(samples=8,use_denoising=True)))
path=folder/'request.json';path.write_text(json.dumps(request))
with (folder/'entrypoint.log').open('wb') as log:
    process=subprocess.Popen([sys.executable,str(root/'work/sfm_export_backend_candidate.py'),str(path)],stdout=log,stderr=subprocess.STDOUT,
        creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    try:
        deadline=time.monotonic()+120;states=[];owned_pid=None;cancelled=False
        while process.poll() is None:
            status=folder/'backend/status.json'
            if status.exists():
                state=json.loads(status.read_text())
                if not states or states[-1]!=state['state']:
                    states.append(state['state']);print('export_entrypoint_state',state['state'],flush=True)
                if state['state']=='converting' and not cancelled:
                    owned_pid=state['pid'];(folder/'backend/cancel').touch();cancelled=True
            if time.monotonic()>deadline:raise TimeoutError('Entry point cancellation timeout')
            time.sleep(.1)
        assert process.returncode==0
        status=json.loads((folder/'backend/status.json').read_text())
        assert cancelled and status['state']=='cancelled' and status['request_id']==identifier
        result=json.loads((folder/'backend/result.json').read_text());assert result['state']=='cancelled'
        assert file_digest(source)==digest==file_digest(captured)
        assert not list((folder/'backend/renders').glob('*/result.json'))
        report=dict(states=states,terminal=status,source_and_capture_unchanged=True,
                    no_render_after_cancel=True,owned_conversion_pid=owned_pid,
                    scope='Actual backend process and private Blender cancellation; export completion and native launch still pending.')
        (root/'outputs/cycles-bridge/export-entrypoint-validation.json').write_text(json.dumps(report,indent=2))
        print('actual_export_entrypoint_cancel_verified',flush=True)
    finally:
        if process.poll() is None:
            (folder/'backend/cancel').touch()
            process.wait(timeout=20)
