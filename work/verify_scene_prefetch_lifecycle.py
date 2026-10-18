from pathlib import Path
import os,subprocess,json
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='1259500',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS='work/four-model-prefetch-ticks.txt',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0',SFM64_DIAGNOSTIC_FRAME_PREFETCH='1')
exe=['build/session-core/probe_vulkan_win32_surface_x64.exe']
run=subprocess.run(exe,env=env,capture_output=True,text=True,timeout=60)
log=run.stdout+run.stderr;Path('outputs/sfm64-scene-prefetch-gpu-only.txt').write_text(log)
assert run.returncode==0,log
assert 'retained Vulkan sequence passed frames=62' in log
assert 'preparation_tasks=61' in log and 'gpu_readback_copies=0 capture_frames=0 async_output_writes=0' in log
bad=Path('work/four-model-unsupported-next-ticks.txt');bad.write_text('1259500\n1392000\n')
env['SFM64_DIAGNOSTIC_SCENE_TICKS']=str(bad.resolve())
run=subprocess.run(exe,env=env,capture_output=True,text=True,timeout=60)
log=run.stdout+run.stderr;Path('outputs/sfm64-scene-prefetch-unsupported-next.txt').write_text(log)
assert run.returncode==1,log
assert 'base texture missing' in log,log
assert 'multi_model_scene_frame=0 tick=1259500 models=4' in log
assert 'multi_model_scene_frame=1' not in log
Path('outputs/sfm64-scene-prefetch-lifecycle.json').write_text(json.dumps(dict(gpu_only_frames=62,readback_copies=0,preparation_tasks=61,unsupported_next_scene_error_propagated=True,scope='process exited with expected error; not native GPU validation or recovery within one running editor instance'),indent=2)+'\n')
print('scene prefetch lifecycle passed: 62 capture-free GPU frames; unsupported next scene reported expected error')
