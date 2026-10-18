import os
import json
import re
import subprocess
import uuid
import hashlib
from pathlib import Path
root=Path(__file__).resolve().parent.parent
source=root/'outputs/obj-gpu-import/session-embedded-obj/resaved-368c7db4-b1e7-434c-8e35-a2c5cff520cb.dmx'
digest=hashlib.sha256(source.read_bytes()).hexdigest()
codec=root/'work/test_render_settings_session.exe'
generated=subprocess.run([str(codec),str(source)],cwd=root,capture_output=True,text=True,check=True)
match=re.search(r'output=(.+)',generated.stdout);assert match,generated.stdout
fixture=root/match[1].strip()
expected=json.loads(subprocess.check_output([str(codec),'--inspect',str(fixture)],cwd=root,text=True))
output=root/'outputs'/('native-render-settings-'+uuid.uuid4().hex[:8]);output.mkdir()
saved=output/'saved.dmx'
exe=root/'build/session-core/probe_vulkan_win32_surface_revision_x64.exe'
env={key:value for key,value in os.environ.items() if not key.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0',SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH=str(saved),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(output))
def run(session,environment,name):
 result=subprocess.run([str(exe),'--session',str(session),'--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test'],cwd=root,env=environment,capture_output=True,text=True,timeout=180)
 text=result.stdout+result.stderr;(output/name).write_text(text)
 assert result.returncode==0 and 'editor_action_rejected=' not in text,text
 assert 'session_render_settings_restored=1' in text,text
 return text
text=run(fixture,env,'save-log.txt')
assert 'editor_session_saved=' in text and 'SFM64 prototype * - Frame' in text and 'SFM64 prototype - Frame' in text
actual=json.loads(subprocess.check_output([str(codec),'--inspect',str(saved)],cwd=root,text=True));assert actual==expected
reopen=env.copy();reopen.pop('SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST');reopen.pop('SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH')
text=run(saved,reopen,'reopen-log.txt');assert 'SFM64 prototype * - Frame' not in text
assert hashlib.sha256(source.read_bytes()).hexdigest()==digest
report=dict(saved_session=str(saved),settings=actual,native_save_as=True,new_process_restore=True,
 saved_baseline_clean=True,original_source_unchanged=True,
 scope='Actual native save/reopen of pre-embedded settings; dialog mutation/failed-save baseline still need separate tests.')
(root/'outputs/native-render-settings-persistence-validation.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
