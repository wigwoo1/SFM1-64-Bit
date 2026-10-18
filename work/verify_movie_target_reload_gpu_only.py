from pathlib import Path
import os,subprocess,json
directory=Path('outputs/movie-target-reload-gpu-only');directory.mkdir(exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS='work/movie-target-reload-ticks.txt',SFM64_DIAGNOSTIC_SCENE_SESSION_SEQUENCE='work/movie-target-reload-sessions.txt',SFM64_DIAGNOSTIC_SCENE_OUTPUT='movie',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0',SFM64_DIAGNOSTIC_FRAME_PREFETCH='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
log=run.stdout+run.stderr;Path('outputs/sfm64-movie-target-reload-gpu-only.txt').write_text(log)
assert run.returncode==0,log
assert 'render_targets=3' in log and 'gpu_readback_copies=0 capture_frames=0 async_output_writes=0' in log
assert 'scene_target_rebuilt=640x480' in log and 'scene_target_rebuilt=3840x2160' in log
assert not list(directory.glob('*.rgba'))
Path('outputs/sfm64-movie-target-reload-gpu-only.json').write_text(json.dumps(dict(frames=3,render_targets=3,resolution_reload=True,gpu_readbacks=0,output_files=0,scope='capture-free execution only; pixel equivalence verified by separate capture runs'),indent=2)+'\n')
print('capture-free movie target reload passed: 3 extents, zero readbacks/output writes')
