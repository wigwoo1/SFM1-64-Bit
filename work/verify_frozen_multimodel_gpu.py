from pathlib import Path
import subprocess,os,json,hashlib
directory=Path('outputs/frozen-four-model-scene');directory.mkdir(exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='1259500',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_COUNT='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
log=run.stdout+run.stderr;Path('outputs/sfm64-frozen-four-model-gpu.txt').write_text(log)
assert run.returncode==0,log
assert 'frozen_scene_models=4 vertices=110396 indices=529608' in log
assert 'Vulkan indexed model draw passed triangles=176536' in log
path=directory/'mamagen-vulkan-blended-draw-v1.rgba';pixels=path.read_bytes();assert len(pixels)==512*512*4
Path('outputs/sfm64-frozen-four-model-gpu.json').write_text(json.dumps(dict(models=4,vertices=110396,indices=529608,pixel_sha256=hashlib.sha256(pixels).hexdigest(),scope='frozen real saved scene GPU0; enabled procedural simulation, native appearance, eye shader and animated multi-model playback unfinished'),indent=2)+'\n')
print('real four-model frozen scene executed on Vulkan GPU; 110396 vertices / 529608 indices; pixels captured')
