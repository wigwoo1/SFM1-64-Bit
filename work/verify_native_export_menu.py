import os
import json
import hashlib
import subprocess
import re
import sys
from pathlib import Path
root=Path(__file__).resolve().parent.parent
source=root/'outputs/obj-gpu-import/session-embedded-obj/resaved-368c7db4-b1e7-434c-8e35-a2c5cff520cb.dmx'
digest=hashlib.sha256(source.read_bytes()).hexdigest()
revision_test=os.environ.get('SFM64_TEST_REQUIRE_REVISION')=='1'
output=root/('outputs/native-export-menu-revision-test' if revision_test else 'outputs/native-export-menu-test');output.mkdir(exist_ok=True)
env={key:value for key,value in os.environ.items() if not key.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',
 SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_EXPORT_MENU_TEST='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(output))
executable=Path(os.environ.get('SFM64_TEST_EXECUTABLE',str(root/'build/session-core/probe_vulkan_win32_surface_x64.exe')))
result=subprocess.run([str(executable),'--session',str(source),
 '--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test'],
 cwd=root,env=env,capture_output=True,text=True,timeout=180)
log=result.stdout+result.stderr;(output/'run.txt').write_text(log)
assert result.returncode==0,log
assert 'editor_action_rejected=' not in log,log
match=re.search(r'native_export_started=(.*?) selected_shot=(.*?) first_frame=1 dirty=1 embedded_obj=1',log)
assert match,log
folder=Path(match[1]);request=json.loads((folder/'request.json').read_text())
assert request['settings']['renderer']=='cycles' and request['settings']['scene']==match[2]
if revision_test:assert request['editor_revision']==1
assert (folder/'cancel').exists() and (folder/'session.dmx').is_file()
assert 'editor_action_applied=1 changed=1 dirty=0' in log,log
assert hashlib.sha256(source.read_bytes()).hexdigest()==digest
pin=json.loads((root/'outputs/cycles-worker/converter-snapshot.json').read_text())
sys.path.insert(0,pin['snapshot']);sys.path.insert(0,str(root/'work'))
from session import Session
from cycles_obj_snapshot_candidate import decode_session_snapshot
original_session=Session(source);captured_session=Session(folder/'session.dmx')
assert decode_session_snapshot(original_session)==decode_session_snapshot(captured_session)
assert source.read_bytes()!=(folder/'session.dmx').read_bytes()
report=dict(request_folder=str(folder),selected_shot=match[2],unsaved_document_captured=True,
 captured_editor_revision=request.get('editor_revision'),
 embedded_obj_snapshot_exact=True,export_preserves_dirty_and_undo=True,source_unchanged=True,
 private_cancel_marker=True,scope='Actual hidden native queued Export/Cancel and snapshot; backend may be stopped by window shutdown before conversion completes.')
(output/'validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
