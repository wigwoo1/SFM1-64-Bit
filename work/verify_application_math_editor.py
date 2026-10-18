import os
from pathlib import Path
import subprocess,os,json
out=Path('outputs/application-math-editor');out.mkdir(exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(out.resolve()))
run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe','--session','work/short-timeline-session.dmx','--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test'],env=env,capture_output=True,text=True,timeout=int(os.environ.get("SFM64_TEST_TIMEOUT_SECONDS","60")))
log=run.stdout+run.stderr;Path('outputs/sfm64-application-math-editor-validation.txt').write_text(log)
assert run.returncode==0,log
assert log.count('application_math_initialized=1 trig_entries=256 gamma_tables=8 seed_blocks=32')==1,log
assert 'editor_action_rejected=' not in log,log
assert 'editor_action_applied=0 changed=1 dirty=1' in log and 'editor_action_applied=1 changed=1 dirty=0' in log and 'editor_action_applied=2 changed=1 dirty=1' in log,log
frames=list(out.glob('vulkan-retained-frame-*.rgba'));assert frames and all(p.stat().st_size==512*512*4 for p in frames)
Path('outputs/sfm64-application-math-editor-validation.json').write_text(json.dumps(dict(math_startup=True,source_session='work/short-timeline-session.dmx',hidden_window=True,editor_actions=['trim','undo','redo'],captured_frames=len(frames),limits='Application math initialized before Vulkan startup; renderer consumers still use their existing math paths. Not full native backend integration or manual UI verification.'),indent=2)+'\n')
print('application_math_editor_startup_and_trim_undo_redo=passed captured_frames='+str(len(frames)))
