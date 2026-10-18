from pathlib import Path
import subprocess,os,json,uuid,hashlib
root=Path(__file__).resolve().parent.parent
directory=root/'outputs'/('live-key-editor-'+uuid.uuid4().hex)
directory.mkdir()
saved=directory/'saved.dmx'
fixture=root/'work/short-timeline-session.dmx'
original=fixture.read_bytes()
exe=root/'build/session-core/probe_vulkan_win32_surface_x64.exe'
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_FLOAT_KEY_TEST='1',SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH=str(saved),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory))
run=subprocess.run([str(exe),'--session',str(fixture),'--content-directory',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test'],env=env,cwd=root,capture_output=True,text=True,timeout=60)
log=run.stdout+run.stderr
(directory/'run.txt').write_text(log)
assert run.returncode==0,log
assert 'float_key_editor_action_applied=2814:0 changed=1 dirty=1' in log,log
assert 'editor_action_applied=1 changed=1 dirty=0' in log,log
assert 'editor_action_applied=2 changed=1 dirty=1' in log,log
assert saved.is_file() and 'action_rejected=' not in log,log
subprocess.run([str(root/'work/test_saved_float_key.exe'),str(fixture),str(saved),'2814','0','123'],check=True)
assert fixture.read_bytes()==original
captures=list(directory.glob('*.rgba'))
assert captures,'No GPU frame captures produced'
report={'saved_session':str(saved),'live_dialog_apply':True,'undo_clears_dirty':True,'redo_restores_dirty':True,'full_saved_graph_preserved_except_key':True,'input_unchanged':True,'gpu_capture_count':len(captures),'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'limits':'Live Vulkan editor action and serialized graph verification. Does not prove the selected key changes visible geometry, native renderer parity, or visual dialog layout.'}
(root/'outputs/sfm64-live-float-key-editor-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('live_float_key_editor Apply/Undo/Redo/Save passed; full saved graph preserved except selected key')
