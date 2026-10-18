"""Result protocol checks; fixture output is not a Blender scene/load test."""
import copy
import json
import uuid
from pathlib import Path
from sfm_cycles_conversion_candidate import accept_conversion, file_digest

root = Path(__file__).resolve().parent.parent
folder = root / 'outputs/cycles-conversion/result-checks' / str(uuid.uuid4())
folder.mkdir(parents=True)
session = folder / 'captured.dmx'
session.write_bytes(b'captured session protocol fixture')
current = folder / 'current.dmx'
current.write_bytes(session.read_bytes())
output = folder / 'session.blend'
output.write_bytes(b'output presence fixture; not a Blender project')
job = dict(output=str(output), session=str(session), session_sha256=file_digest(session), run_id='fixture')
prepared = dict(folder=str(folder), job=job)
report = dict(run_id='fixture', status='partial', warnings=['Known approximation'],
              unsupported=['Missing texture'], errors=[],
              bridge=dict(background_only=True, captured_session_sha256=job['session_sha256']),
              shots=[dict(name='actual shot', frame_start=7, frame_end=9)])
report_path = output.with_suffix('.report.json')
def write(value):
    report_path.write_text(json.dumps(value))
write(report)
accepted = accept_conversion(prepared, 0, current)
assert accepted['fidelity'] == 'partial'
assert accepted['report']['unsupported'] == ['Missing texture']
rejections = []
for field, value in [('run_id', 'wrong'), ('status', 'failed'),
                     ('errors', ['Hierarchy conversion failed']),
                     ('complete_animation_verified', False), ('shots', [])]:
    invalid = copy.deepcopy(report)
    invalid[field] = value
    write(invalid)
    try:
        accept_conversion(prepared, 0, current)
    except ValueError:
        rejections.append(field)
    else:
        raise AssertionError('Accepted invalid ' + field)
write(report)
expected_obj = dict(scale=.5, polygons=2, materials=['left','right'])
try:
    accept_conversion(prepared, 0, expected_imported_obj=expected_obj)
except ValueError:
    pass
else:
    raise AssertionError('Embedded OBJ omission was accepted')
with_obj = copy.deepcopy(report)
with_obj['bridge']['imported_obj'] = dict(object='Imported OBJ',shots=['actual shot'],**expected_obj)
write(with_obj)
assert accept_conversion(prepared,0,expected_imported_obj=expected_obj)['fidelity']=='partial'
with_obj['bridge']['imported_obj']['shots']=[]
write(with_obj)
try:
    accept_conversion(prepared,0,expected_imported_obj=expected_obj)
except ValueError:
    pass
else:
    raise AssertionError('Incomplete OBJ shot coverage was accepted')
write(report)
current.write_bytes(b'edited current session')
try:
    accept_conversion(prepared, 0, current)
except ValueError:
    rejections.append('stale_current_session')
else:
    raise AssertionError('Accepted stale preview')
assert accept_conversion(prepared, 0)['report']['warnings'] == report['warnings']
evidence = dict(partial_fidelity_retained=True, rejected=rejections,
    embedded_obj_omission_and_incomplete_shots_rejected=True,
    scope='Terminal conversion result protocol only; real Blender load/render tested separately.')
(root / 'outputs/cycles-conversion/result-acceptance-validation.json').write_text(json.dumps(evidence, indent=2))
print('conversion_result_acceptance_checks_passed')
