from pathlib import Path
import os,subprocess,struct,json
matrix=[1,0,0,1000000,0,1,0,0,0,0,1,0,0,0,0,1]
path=Path('work/offscreen-scene-camera.bin');path.write_bytes(struct.pack('<16f',*matrix))
directory=Path('outputs/offscreen-scene');directory.mkdir(exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_CAMERA_MATRIX=str(path),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
log=run.stdout+run.stderr;Path('outputs/sfm64-offscreen-scene-gpu.txt').write_text(log)
assert run.returncode==0,log
assert 'triangles=66146 model_visible=1 changed_pixels=0' in log
pixels=(directory/'mamagen-vulkan-session-camera-v1.rgba').read_bytes()
assert pixels==bytes([13,13,20,255])*(512*512)
Path('outputs/sfm64-offscreen-scene-gpu.json').write_text(json.dumps(dict(visible_model=True,triangles=66146,clipped_output_pixel_exact=True,scope='GPU0 controlled finite camera matrix, no native camera or visibility parity claim'),indent=2)+'\n')
print('valid offscreen scene accepted: visible model and triangles present, clipped output exact clear')
