from pathlib import Path
import subprocess,os,re,json
directory=Path('outputs/live-editor-controls');directory.mkdir(exist_ok=True);env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')};env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe','--session','work/short-timeline-session.dmx','--content-directory',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod','--control-test'],env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;Path('outputs/sfm64-live-editor-controls.txt').write_text(log,encoding='utf-8');assert run.returncode==0,log
assert [int(n) for n in re.findall(r'multi_model_scene_frame=(\d+) tick=',log)]==[0,1,1,1,1]
assert re.findall(r'editor_action_applied=(\d+) changed=(\d+) dirty=(\d+) frame=(\d+)',log)==[('0','1','1','1'),('1','1','0','1'),('2','1','1','1')]
assert 'editor_action_rejected=' not in log and 'retained_gpu_material_cache_entries=1' in log
for i in (0,1):assert (directory/f'vulkan-retained-frame-{i}.rgba').read_bytes()==Path(f'outputs/auto-timeline-1-prefetch-0/vulkan-retained-frame-{i}.rgba').read_bytes()
Path('outputs/sfm64-live-editor-gpu.json').write_text(json.dumps(dict(real_Win32_commands=['trim','undo','redo'],in_memory_document=True,pixels_exact=True,material_cache_entries=1,dirty_states=[True,False,True],scope='hidden frame-boundary edit test, no manual UI verification'),indent=2),encoding='utf-8');print('live editor Vulkan passed in-memory trim/undo/redo at frame boundary, dirty states correct, pixels exact, material cache reused')
