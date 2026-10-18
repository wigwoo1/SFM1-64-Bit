"""Private Blender entry point for conversion inside the renderer bridge."""
import json
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from sfm_cycles_conversion_candidate import file_digest

job_path = Path(sys.argv[sys.argv.index('--') + 1])
job = json.loads(job_path.read_text(encoding='utf-8'))
snapshot = Path(job['converter_snapshot']).resolve()
manifest_path = snapshot / 'snapshot-manifest.json'
if file_digest(manifest_path) != job['converter_manifest_sha256']:
    raise ValueError('Converter manifest changed after job preparation')
manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
for relative, record in manifest['files'].items():
    path = (snapshot / relative).resolve()
    if not path.is_relative_to(snapshot) or file_digest(path) != record['sha256']:
        raise ValueError('Converter changed after job preparation: ' + relative)
if file_digest(job['session']) != job['session_sha256']:
    raise ValueError('Captured session changed after job preparation')
sys.path.insert(0, str(snapshot))
import blender_worker as converter
import outliner_state
from cycles_obj_snapshot_candidate import decode_session_snapshot, insert_snapshot_in_scenes

captured_snapshot = None
OriginalSession = converter.Session
class BridgeSession(OriginalSession):
    def __init__(self, path):
        global captured_snapshot
        super().__init__(path)
        captured_snapshot = decode_session_snapshot(self)
converter.Session = BridgeSession
original_event = converter.event
def bridge_event(stage, message, progress=None):
    if stage == 'pack' and captured_snapshot is not None:
        import bpy
        scenes = [bpy.data.scenes[shot['name']] for shot in converter.REPORT['shots']]
        obj = insert_snapshot_in_scenes(captured_snapshot, scenes)
        converter.REPORT['bridge']['imported_obj'] = dict(object=obj.name,
            shots=[scene.name for scene in scenes], scale=captured_snapshot['scale'],
            polygons=len(captured_snapshot['polygons']), materials=list(captured_snapshot['images']))
        limitation = 'Imported OBJ normal/smoothing and ngon triangulation parity remain unverified.'
        converter.warn(limitation)
        converter.REPORT['unsupported'].append(limitation)
    return original_event(stage, message, progress)
converter.event = bridge_event

def internal_open_state(blender, blend):
    return dict(skipped=True, reason='Renderer bridge consumes the saved scene in background; Outliner UI finalization is unnecessary.')

# The converter prepares saved scenes itself. Its optional Outliner step starts
# a second UI Blender; the internal bridge needs only the scene data.
outliner_state.finish_file = internal_open_state
converter.REPORT['run_id'] = job['run_id']
converter.JOB = job
converter.REPORT['bridge'] = dict(background_only=True, captured_session_sha256=job['session_sha256'])
try:
    converter.convert(job)
except Exception as error:
    converter.REPORT['status'] = 'failed'
    converter.REPORT['errors'].append(str(error))
    Path(job['output']).with_suffix('.report.json').write_text(
        json.dumps(converter.REPORT, indent=2), encoding='utf-8')
    raise
