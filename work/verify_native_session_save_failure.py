"""Actual native failed Save As regression; no Blender job is requested."""
import hashlib
import json
import os
import subprocess
import uuid
from pathlib import Path

root=Path(__file__).resolve().parent.parent
source=root/'outputs/obj-gpu-import/session-embedded-obj/resaved-368c7db4-b1e7-434c-8e35-a2c5cff520cb.dmx'
before=hashlib.sha256(source.read_bytes()).hexdigest()
output=root/'outputs'/('native-save-failure-'+uuid.uuid4().hex[:8])
output.mkdir()
destination=output/'directory.dmx'
destination.mkdir()
sentinel=destination/'preserve.txt'
sentinel.write_bytes(b'Existing destination must survive failed Save As')
environment={key:value for key,value in os.environ.items() if not key.startswith('SFM64_DIAGNOSTIC_')}
environment.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0',
 SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH=str(destination),
 SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(output))
result=subprocess.run([str(root/'build/session-core/probe_vulkan_win32_surface_revision_x64.exe'),
 '--session',str(source),'--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',
 '--control-test'],cwd=root,env=environment,capture_output=True,text=True,timeout=180)
log=result.stdout+result.stderr
(output/'native-log.txt').write_text(log)
assert result.returncode==0,log
assert log.count('editor_action_rejected=')==1 and 'editor_session_saved=' not in log,log
failure=log.index('editor_action_rejected=')
titles=[line for line in log[failure:].splitlines() if line.startswith('playback_window_title=')]
assert titles and all('prototype * - Frame' in line for line in titles),log
assert destination.is_dir() and sentinel.read_bytes()==b'Existing destination must survive failed Save As'
assert sorted(path.name for path in destination.iterdir())==['preserve.txt']
assert not list(output.glob('.sfm64-save-*'))
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
report=dict(native_exit_code=result.returncode,save_failure_reported=True,unsaved_title_retained=True,
 original_source_unchanged=True,destination_preserved=True,log=str(output/'native-log.txt'),
 scope='Actual native Save As destination preflight rejection with timeline edits; disk-write/commit failure and changed render-settings baseline remain separately unverified.')
(output/'validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
