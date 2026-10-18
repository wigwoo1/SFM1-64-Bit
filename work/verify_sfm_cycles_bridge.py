"""Full actual Source session/embedded OBJ through both private controllers."""
import json
import sys
import time
from pathlib import Path
from sfm_cycles_conversion_candidate import prepare_conversion,file_digest
from sfm_cycles_bridge_candidate import SfmCyclesBridgeCandidate
root=Path(__file__).resolve().parent.parent
pin=json.loads((root/'outputs/cycles-worker/converter-snapshot.json').read_text())
sys.path.insert(0,pin['snapshot'])
from session import Session
from cycles_obj_snapshot_candidate import decode_session_snapshot
blender=r'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
source=root/'outputs/obj-gpu-import/session-embedded-obj/resaved-368c7db4-b1e7-434c-8e35-a2c5cff520cb.dmx'
before=file_digest(source)
prepared=prepare_conversion(source,r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game',
    pin['snapshot'],root/'work/cj',blender)
snapshot=decode_session_snapshot(Session(prepared['job']['session']))
expected=dict(scale=snapshot['scale'],polygons=len(snapshot['polygons']),materials=list(snapshot['images']))
bridge=SfmCyclesBridgeCandidate(blender,root/'work/cj',root/'outputs/cycles-bridge/renders')
try:
    settings=dict(resolution=[256,256],cycles=dict(device='CPU',samples=8,
        use_adaptive_sampling=True,adaptive_threshold=.05,use_denoising=True))
    identifier=bridge.start(prepared,settings,expected,source)
    settings['cycles']['samples']=123
    assert bridge.jobs[identifier]['settings']['cycles']['samples']==8
    folder=root/'outputs/cycles-bridge';folder.mkdir(parents=True,exist_ok=True)
    (folder/'active-job.json').write_text(json.dumps(prepared,indent=2))
    print('bridge_job_started',identifier,flush=True)
    started=last_update=time.monotonic();states=[]
    while True:
        outcome=bridge.poll(identifier)
        state=outcome['state']
        if not states or states[-1]!=state:
            states.append(state);print('bridge_state',state,flush=True)
        if state not in ('converting','rendering','cancelling'):break
        if time.monotonic()-started>2400:raise TimeoutError('Bridge full-session verification timeout')
        if time.monotonic()-last_update>30:
            print('bridge_live',state,identifier,flush=True);last_update=time.monotonic()
        time.sleep(.2)
    assert file_digest(source)==before
    report=dict(states=states,outcome=outcome,source_unchanged=True,immutable_render_settings=True,
        limitation='Backend state machine only; native editor switching, Source parity and export UI unfinished.')
    (folder/'validation.json').write_text(json.dumps(report,indent=2))
    assert state=='complete',outcome
    assert 'converting' in states and 'rendering' in states
    assert not bridge.cancel(identifier)
    print('full_source_cycles_bridge_verified',flush=True)
finally:bridge.close()
