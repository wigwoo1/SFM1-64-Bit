"""Check prepared inputs remain independent and damaged pins cannot launch."""
import hashlib
import json
import uuid
from pathlib import Path
from sfm_cycles_conversion_candidate import prepare_conversion, file_digest

workspace = Path(__file__).resolve().parent.parent
folder = workspace / 'outputs/cycles-conversion/input-checks' / str(uuid.uuid4())
pin = folder / 'pin'
pin.mkdir(parents=True)
worker = pin / 'blender_worker.py'
worker.write_bytes(b'# private test fixture; never executed\n')
manifest = dict(coherent=True, files={'blender_worker.py':
    dict(bytes=worker.stat().st_size, sha256=file_digest(worker))})
manifest_path = pin / 'snapshot-manifest.json'
manifest_path.write_text(json.dumps(manifest))
source = folder / 'source.dmx'
source.write_bytes((workspace / 'work/short-timeline-session.dmx').read_bytes())
content = folder / 'content'
content.mkdir()
blender = r'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
prepared = prepare_conversion(source, content, pin, folder / 'jobs', blender)
captured = Path(prepared['job']['session'])
expected = prepared['provenance']['session_sha256']
source.write_bytes(b'caller changed the session after preparation')
assert file_digest(captured) == expected
assert file_digest(source) != expected
archive = folder / 'extra.VPK'
archive.write_bytes(b'preparation fixture; archive contents are not tested here')
archive_job = prepare_conversion(source, content, pin, folder / 'archive-jobs', blender,
                                 extra_assets=[content, archive])
assert archive_job['job']['extra_assets'] == [str(content.resolve()), str(archive.resolve())]
invalid_asset = folder / 'extra.txt'
invalid_asset.write_text('not an asset archive')
try:
    prepare_conversion(source, content, pin, folder / 'invalid-asset', blender,
                       extra_assets=[invalid_asset])
except ValueError:
    pass
else:
    raise AssertionError('Unrecognized asset file was accepted')
assert not (folder / 'invalid-asset').exists()
worker.write_bytes(b'tampered converter')
try:
    prepare_conversion(source, content, pin, folder / 'rejected', blender)
except ValueError as error:
    tamper_error = str(error)
else:
    raise AssertionError('Damaged converter was accepted')
assert not (folder / 'rejected').exists()
manifest['files'] = {'../source.dmx': dict(bytes=source.stat().st_size, sha256=file_digest(source))}
manifest_path.write_text(json.dumps(manifest))
try:
    prepare_conversion(source, content, pin, folder / 'escaped', blender)
except ValueError as error:
    escape_error = str(error)
else:
    raise AssertionError('Escaping converter path was accepted')
assert not (folder / 'escaped').exists()
evidence = dict(captured_session_survives_caller_edit=True,
    directory_and_vpk_inputs_retained=True, unrelated_asset_file_rejected=True,
    damaged_pin_rejected_before_job_creation=tamper_error,
    escaping_pin_rejected_before_job_creation=escape_error,
    scope='Preparation validation only; fixture converter was never executed.')
(folder / 'report.json').write_text(json.dumps(evidence, indent=2))
(workspace / 'outputs/cycles-conversion/input-capture-validation.json').write_text(json.dumps(evidence, indent=2))
print('conversion_input_capture_checks_passed')
