"""Full actual native menu -> edited capture -> conversion -> 1080p render."""
import os
import json
import hashlib
import subprocess
import time
import re
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parent.parent
source=root/'outputs/obj-gpu-import/session-embedded-obj/resaved-368c7db4-b1e7-434c-8e35-a2c5cff520cb.dmx'
before=hashlib.sha256(source.read_bytes()).hexdigest()
preview_test=os.environ.get('SFM64_TEST_PREVIEW')=='1'
folder=root/('outputs/native-export-preview-complete' if preview_test else 'outputs/native-export-complete');folder.mkdir(exist_ok=True)
env={key:value for key,value in os.environ.items() if not key.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0',
 SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_EXPORT_COMPLETE_TEST='1',
 SFM64_DIAGNOSTIC_STARTUP_TRACE='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(folder))
if preview_test:env['SFM64_DIAGNOSTIC_EXPORT_PREVIEW_TEST']='1'
args=[str(root/'build/session-core/probe_vulkan_win32_surface_x64.exe'),'--session',str(source),
 '--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test']
with (folder/'run.txt').open('w') as log:
 process=subprocess.Popen(args,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
 print('native_full_export_process',process.pid,flush=True)
 started=last=time.monotonic();request_folder=None
 try:
  while process.poll() is None:
   if request_folder is None:
    match=re.search(r'native_export_started=(.*?) selected_shot=',(folder/'run.txt').read_text())
    if match:
     request_folder=Path(match[1]);(folder/'active-request.json').write_text(json.dumps(dict(folder=str(request_folder),native_pid=process.pid)))
     print('native_full_request',str(request_folder),flush=True)
   if time.monotonic()-last>30:
    status=request_folder/'backend/status.json' if request_folder else None
    print('native_full_live',json.loads(status.read_text()).get('state') if status and status.exists() else 'native startup',flush=True);last=time.monotonic()
   if time.monotonic()-started>3600:raise TimeoutError('Full native export timeout')
   time.sleep(.2)
  assert process.returncode==0
  assert request_folder is not None
  status=json.loads((request_folder/'backend/status.json').read_text());result=json.loads((request_folder/'backend/result.json').read_text())
  assert status['state']==result['state']=='complete'
  request=json.loads((request_folder/'request.json').read_text())
  assert status['request_id']==request['request_id']
  image=Image.open(status['output_png']);image.load();assert image.size==(1920,1080)
  assert hashlib.sha256(source.read_bytes()).hexdigest()==before
  text=(folder/'run.txt').read_text();assert 'dirty=1 embedded_obj=1' in text and 'editor_action_applied=1 changed=1 dirty=0' in text and 'editor_action_rejected=' not in text
  if preview_test:
   assert request['editor_revision']==1
   assert 'native_export_preview_verified=1 revision=1' in text and 'native_export_preview_removed_after_undo=1' in text
   receipts=list((request_folder/'backend').glob('accepted-*/accepted.json'));assert len(receipts)==1
   receipt=json.loads(receipts[0].read_text());assert receipt['request_id']==request['request_id'] and receipt['editor_revision']==1
   assert hashlib.sha256(Path(receipt['presentation_png']).read_bytes()).hexdigest()==receipt['output_sha256']
  report=dict(status=status,source_unchanged=True,native_dirty_capture_and_undo=True,resolution_verified=[1920,1080],
   accepted_preview_and_stale_window_close=preview_test,
   scope=('Actual hidden native export/acceptance/preview/revision test; fidelity partial and movie export unfinished.' if preview_test else 'Actual native first-frame export; fidelity partial, image presentation and movie export unfinished.'))
  (folder/'validation.json').write_text(json.dumps(report,indent=2));print('native_full_1080p_export_verified',flush=True)
 finally:
  if process.poll() is None:process.terminate();process.wait(timeout=15)
