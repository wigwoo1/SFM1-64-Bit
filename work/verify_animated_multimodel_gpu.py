from pathlib import Path
import os,subprocess,json,hashlib,re
ticks=[1259500+(n*10000)//60 for n in range(61)]+[1259500]
tick_path=Path('work/four-model-animation-ticks.txt')
tick_path.write_text('\n'.join(map(str,ticks))+'\n')
directory=Path('outputs/animated-four-model-scene');directory.mkdir(exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK=str(ticks[0]),SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_SCENE_TICKS=str(tick_path.resolve()),SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
log=run.stdout+run.stderr;Path('outputs/sfm64-animated-four-model-gpu.txt').write_text(log)
assert run.returncode==0,log
assert [(int(a),int(b),int(c)) for a,b,c in re.findall(r'multi_model_scene_frame=(\d+) tick=(\d+) models=(\d+)',log)]==[(n,t,4) for n,t in enumerate(ticks)]
assert f'retained Vulkan sequence passed frames={len(ticks)} available_frames={len(ticks)} geometry_allocations=1 material_uploads=1' in log
pixels=[(directory/f'vulkan-retained-frame-{n}.rgba').read_bytes() for n in range(len(ticks))]
assert all(len(p)==512*512*4 for p in pixels)
assert pixels[0]==pixels[-1],'backward seek pixels changed'
assert any(p!=pixels[0] for p in pixels[1:-1]),'animated output stayed frozen'
serial_directory=Path('outputs/animated-four-model-scene-serial');serial_directory.mkdir(exist_ok=True)
env.update(SFM64_DIAGNOSTIC_GEOMETRY_WORKERS='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(serial_directory.resolve()))
serial=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
serial_log=serial.stdout+serial.stderr;Path('outputs/sfm64-animated-four-model-gpu-serial.txt').write_text(serial_log)
assert serial.returncode==0,serial_log
assert 'geometry_worker_count=1' in serial_log
for n,p in enumerate(pixels):
 assert p==(serial_directory/f'vulkan-retained-frame-{n}.rgba').read_bytes(),f'geometry worker count changed frame {n}'
report=dict(models=4,ticks=ticks,frames=len(ticks),backward_seek_pixel_exact=True,animation_changes_pixels=True,one_and_four_workers_pixel_exact=True,frame_sha256=[hashlib.sha256(p).hexdigest() for p in pixels],scope='GPU0 animated saved scene; topology/resource transitions, full native shading, eyes, enabled procedural simulation and editor unfinished')
Path('outputs/sfm64-animated-four-model-gpu.json').write_text(json.dumps(report,indent=2)+'\n')
print('four-model Vulkan animation passed: 62 frames, retained resources, changing pixels, backward seek and one/four workers pixel exact')
