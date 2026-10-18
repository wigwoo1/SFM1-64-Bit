"""Verify an owned full-session conversion through the Cycles controller.

Conversion warnings remain part of the evidence; successful rendering does not
establish Source effect fidelity or integration with the native editor.
"""
import hashlib
import argparse
import sys
import json
import time
from pathlib import Path
from cycles_job_controller_candidate import CyclesJobControllerCandidate
from sfm_cycles_conversion_candidate import accept_conversion

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser()
parser.add_argument('--prepared', type=Path)
parser.add_argument('--conversion-exit-code', type=int)
parser.add_argument('--require-imported-obj', action='store_true')
arguments = parser.parse_args()
if arguments.require_imported_obj and arguments.prepared is None:
    parser.error('Embedded OBJ validation requires a prepared job')
accepted = None
if arguments.prepared is not None:
    if arguments.conversion_exit_code is None:
        parser.error('Prepared jobs require the observed terminal conversion exit code')
    prepared = json.loads(arguments.prepared.read_text(encoding='utf-8'))
    expected_obj = None
    if arguments.require_imported_obj:
        sys.path.insert(0, prepared['job']['converter_snapshot'])
        from session import Session
        from cycles_obj_snapshot_candidate import decode_session_snapshot
        snapshot = decode_session_snapshot(Session(prepared['job']['session']))
        if snapshot is None:
            raise ValueError('Expected OBJ session has no embedded snapshot')
        expected_obj = dict(scale=snapshot['scale'], polygons=len(snapshot['polygons']),
                            materials=list(snapshot['images']))
    accepted = accept_conversion(prepared, arguments.conversion_exit_code,
                                 prepared['provenance']['source_session'], expected_obj)
    folder = Path(prepared['folder'])
else:
    folder = root / 'outputs/cycles-conversion/short-path-session'
job = json.loads((folder / 'job.json').read_text(encoding='utf-8-sig'))
report = json.loads((folder / 'session.report.json').read_text(encoding='utf-8'))
if report.get('run_id') != job['run_id']:
    raise ValueError('Conversion report identity mismatch')
if report.get('status') not in ('partial', 'converted_with_warnings', 'converted'):
    raise ValueError('Conversion did not produce an acceptable candidate')
if report.get('complete_animation_verified') is False:
    raise ValueError('Diagnostic preview conversion is not a full-session candidate')
blend = Path(job['output'])
if not blend.is_file() or blend.stat().st_size == 0:
    raise ValueError('Missing converted project')
shots = report.get('shots', [])
if not shots or not isinstance(shots[0].get('name'), str):
    raise ValueError('Conversion report has no named shot to verify')
first_shot = shots[0]
controller = CyclesJobControllerCandidate(
    r'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe', folder / 'renders')
try:
    identifier = controller.start(dict(version=1, mode='render', input_blend=str(blend),
        scene=first_shot['name'], frame=first_shot['frame_start'], resolution=[256, 256], cycles=dict(device='CPU',
        samples=8, use_adaptive_sampling=True, adaptive_threshold=.05, use_denoising=True)))
    started = time.monotonic()
    last_update = started
    while True:
        outcome = controller.poll(identifier)
        if outcome['state'] not in ('running', 'cancelling'):
            break
        if time.monotonic() - started > 600:
            raise TimeoutError('Render verification exceeded ten minutes')
        if time.monotonic() - last_update >= 15:
            print('render_verification_running', identifier, flush=True)
            last_update = time.monotonic()
        time.sleep(.2)
    evidence = dict(conversion_status=report['status'], conversion_run_id=job['run_id'],
        internal_bridge_result_validated=accepted is not None,
        counts=report.get('counts'), warnings=report.get('warnings'),
        errors=report.get('errors'), unsupported=report.get('unsupported'),
        snapshot=json.loads((root / 'outputs/cycles-worker/converter-snapshot.json').read_text()),
        source_session_sha256=hashlib.sha256(Path(job['session']).read_bytes()).hexdigest(),
        render=outcome, limitation='Backend conversion/render evidence only; Source parity and editor integration unverified.')
    (folder / 'pipeline-verification.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    if outcome['state'] != 'complete':
        raise RuntimeError(outcome)
    print('pinned_session_conversion_render_verified', flush=True)
finally:
    controller.close()
