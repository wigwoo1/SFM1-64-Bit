"""Prepare a private, pinned converter job for the future renderer bridge.

The session is captured; installed assets remain read-only external inputs.
This module does not provide editor integration or conversion cancellation.
"""
import hashlib
import json
import uuid
from pathlib import Path

def file_digest(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(8 * 1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()

def accept_conversion(prepared, exit_code, current_session=None, expected_imported_obj=None):
    """Accept a terminal private worker result, retaining partial-fidelity data."""
    if exit_code != 0:
        raise ValueError('Conversion worker did not finish successfully')
    job = prepared['job']
    folder = Path(prepared['folder']).resolve()
    output = Path(job['output']).resolve()
    if output != folder / 'session.blend':
        raise ValueError('Conversion output is not the private job output')
    if file_digest(job['session']) != job['session_sha256']:
        raise ValueError('Captured session changed during conversion')
    if current_session is not None and file_digest(current_session) != job['session_sha256']:
        raise ValueError('Conversion result is stale for the current session')
    report = json.loads(output.with_suffix('.report.json').read_text(encoding='utf-8'))
    if report.get('run_id') != job['run_id']:
        raise ValueError('Conversion result identity mismatch')
    bridge = report.get('bridge', {})
    if bridge.get('background_only') is not True or bridge.get('captured_session_sha256') != job['session_sha256']:
        raise ValueError('Conversion result does not match the captured bridge input')
    if report.get('status') not in ('converted', 'converted_with_warnings', 'partial'):
        raise ValueError('Conversion report indicates failure')
    if not isinstance(report.get('errors'), list) or report['errors']:
        raise ValueError('Conversion contains errors; project fidelity is incomplete')
    if report.get('complete_animation_verified') is False:
        raise ValueError('Diagnostic preview conversion cannot replace a full project')
    if not output.is_file() or output.stat().st_size == 0:
        raise ValueError('Missing converted project')
    shots = report.get('shots', [])
    if not shots:
        raise ValueError('Conversion has no renderable shots')
    for shot in shots:
        if not isinstance(shot.get('name'), str) or not shot['name']:
            raise ValueError('Invalid converted shot name')
        for field in ('frame_start', 'frame_end'):
            if type(shot.get(field)) is not int:
                raise ValueError('Invalid converted shot frame range')
        if shot['frame_end'] < shot['frame_start']:
            raise ValueError('Reversed converted shot frame range')
    if expected_imported_obj is not None:
        imported = bridge.get('imported_obj')
        if not isinstance(imported, dict) or not isinstance(imported.get('object'), str) or not imported['object']:
            raise ValueError('Conversion report omitted the embedded OBJ')
        if imported.get('shots') != [shot['name'] for shot in shots]:
            raise ValueError('Embedded OBJ is not reported in every converted shot')
        for field in ('scale','polygons','materials'):
            if imported.get(field) != expected_imported_obj[field]:
                raise ValueError('Embedded OBJ result mismatch: ' + field)
    return dict(input_blend=str(output), shots=shots, report=report,
                fidelity=report['status'], run_id=job['run_id'])

def prepare_conversion(session, sfm, snapshot, root, blender, extra_assets=()):
    session, sfm, snapshot, root, blender = map(
        lambda value: Path(value).resolve(), (session, sfm, snapshot, root, blender))
    if not session.is_file() or not blender.is_file() or not sfm.is_dir():
        raise ValueError('Missing session, Blender executable, or SFM content root')
    manifest_path = snapshot / 'snapshot-manifest.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    if manifest.get('coherent') is not True or not manifest.get('files'):
        raise ValueError('Converter snapshot has no coherent file manifest')
    for relative, record in manifest['files'].items():
        path = (snapshot / relative).resolve()
        if not path.is_relative_to(snapshot):
            raise ValueError('Converter manifest path escapes snapshot')
        if path.stat().st_size != record['bytes'] or file_digest(path) != record['sha256']:
            raise ValueError('Converter snapshot integrity mismatch: ' + relative)
    worker = snapshot / 'blender_worker.py'
    if 'blender_worker.py' not in manifest['files']:
        raise ValueError('Converter worker is not pinned')
    assets = [str(Path(value).resolve()) for value in extra_assets]
    if any(not (Path(value).is_dir() or
                (Path(value).is_file() and Path(value).suffix.lower() == '.vpk'))
           for value in assets):
        raise ValueError('Additional assets must be existing directories or VPK archives')
    identifier = str(uuid.uuid4())
    folder = root / identifier[:8]
    folder.mkdir(parents=True)
    captured = folder / 'session.dmx'
    with session.open('rb') as source, captured.open('xb') as destination:
        for chunk in iter(lambda: source.read(8 * 1024 * 1024), b''):
            destination.write(chunk)
    digest = file_digest(captured)
    if file_digest(session) != digest:
        raise ValueError('Session changed while capturing conversion input')
    job = dict(session=str(captured), sfm=str(sfm), output=str(folder / 'session.blend'),
               run_id=identifier, extra_assets=assets, converter_snapshot=str(snapshot),
               converter_manifest_sha256=file_digest(manifest_path), session_sha256=digest)
    job_path = folder / 'job.json'
    job_path.write_text(json.dumps(job, indent=2), encoding='utf-8')
    provenance = dict(run_id=identifier, source_session=str(session),
        captured_session=str(captured), session_sha256=digest,
        converter_manifest=str(manifest_path), converter_manifest_sha256=file_digest(manifest_path),
        external_assets_captured=False)
    (folder / 'provenance.json').write_text(json.dumps(provenance, indent=2), encoding='utf-8')
    return dict(job=job, folder=str(folder), provenance=provenance,
        args=[str(blender), '--background', '--factory-startup', '--disable-autoexec',
              '--threads', '2', '--python-exit-code', '1', '--python',
              str(Path(__file__).with_name('sfm_cycles_conversion_worker_candidate.py').resolve()),
              '--', str(job_path)])

if __name__ == '__main__':
    workspace = Path(__file__).resolve().parent.parent
    pin = json.loads((workspace / 'outputs/cycles-worker/converter-snapshot.json').read_text())
    prepared = prepare_conversion(workspace / 'work/short-timeline-session.dmx',
        r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game', pin['snapshot'],
        workspace / 'work/cj',
        r'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe')
    (workspace / 'outputs/cycles-conversion/prepared-job.json').write_text(
        json.dumps(prepared, indent=2), encoding='utf-8')
    print('private_conversion_input_prepared', prepared['job']['run_id'])
