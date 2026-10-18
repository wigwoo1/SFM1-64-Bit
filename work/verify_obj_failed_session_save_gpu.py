import os,json,subprocess,hashlib
from pathlib import Path
out=Path('outputs/obj-gpu-import/session-embedded-obj');state=json.loads((out/'report.json').read_text());session=Path(state['session']);before=session.read_bytes();folder=out/'save-failure';folder.mkdir(exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')};env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OBJ_SAVE_FAILURE_TEST='1',SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH=str(session.resolve()),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(folder.resolve()))
args=['build/session-core/probe_vulkan_win32_surface_x64.exe','--session',str(session),'--content-directory',r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test'];run=subprocess.run(args,env=env,capture_output=True,text=True,timeout=int(os.environ.get('SFM64_TEST_TIMEOUT_SECONDS','240')));log=run.stdout+run.stderr;(folder/'log.txt').write_text(log);assert run.returncode==0,log
assert 'editor_action_rejected=save destination exists or invalid' in log and 'editor_session_saved=' not in log
assert session.read_bytes()==before,'failed save changed original session'
frames=[(folder/f'vulkan-retained-submission-{i}.rgba').read_bytes() for i in range(6)];assert frames[1]==frames[4] and frames[2]==frames[3]==frames[5] and frames[1]!=frames[2]
titles=[line for line in log.splitlines() if line.startswith('playback_window_title=')];assert len(titles)==6,titles
assert ['prototype *' in title for title in titles]==[False,False,True,True,False,True],titles
report=dict(actual_gpu=True,save_existing_destination_rejected=True,original_session_sha256=hashlib.sha256(before).hexdigest(),original_session_unchanged=True,failed_save_retains_unsaved_state=True,undo_returns_saved_clean_state=True,redo_restores_unsaved_state=True,exact_gpu_pixels=True)
(folder/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('obj_failed_session_save_gpu=passed '+str(report))
