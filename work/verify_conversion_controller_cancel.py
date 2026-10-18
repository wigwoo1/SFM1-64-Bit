import json
import time
from pathlib import Path
from sfm_cycles_conversion_candidate import prepare_conversion,file_digest
from sfm_conversion_job_controller_candidate import ConversionJobControllerCandidate
root=Path(__file__).resolve().parent.parent
blender=r'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
source=root/'work/short-timeline-session.dmx'
before=file_digest(source)
pin=json.loads((root/'outputs/cycles-worker/converter-snapshot.json').read_text())
prepared=prepare_conversion(source,r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game',
    pin['snapshot'],root/'work/cj',blender)
controller=ConversionJobControllerCandidate(blender,root/'work/cj')
try:
    identifier=controller.start(prepared)
    captured_output=controller.jobs[identifier]['prepared']['job']['output']
    prepared['job']['output']='caller changed parameters'
    assert controller.jobs[identifier]['prepared']['job']['output']==captured_output
    started=time.monotonic()
    while True:
        outcome=controller.poll(identifier)
        assert outcome['state']=='running',outcome
        log=Path(outcome.get('log',controller.jobs[identifier]['folder']/'controller-worker.log'))
        if log.exists() and 'SFM_EVENT' in log.read_text(errors='replace'):break
        if time.monotonic()-started>120:raise TimeoutError('Conversion startup timeout')
        time.sleep(.2)
    signal_start=time.monotonic()
    assert controller.cancel(identifier)
    signal_seconds=time.monotonic()-signal_start
    while controller.poll(identifier)['state']=='cancelling':
        if time.monotonic()-signal_start>15:raise TimeoutError('Cancellation did not terminate')
        time.sleep(.1)
    outcome=controller.poll(identifier)
    assert outcome['state']=='cancelled'
    assert controller.jobs[identifier]['process'].poll() is not None
    assert file_digest(source)==before
    (root/'outputs/cycles-conversion/conversion-controller-validation.json').write_text(json.dumps(
        dict(cancelled_owned_live_worker=True,immutable_parameters=True,source_unchanged=True,
             signal_seconds=signal_seconds,outcome=outcome),indent=2))
    print('conversion_controller_live_cancel_verified',flush=True)
finally:controller.close()
