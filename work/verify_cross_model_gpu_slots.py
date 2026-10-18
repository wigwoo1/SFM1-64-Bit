from pathlib import Path
import subprocess,os,json
directory=Path('outputs/cross-model-gpu-slots');directory.mkdir(exist_ok=True)
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_PRESENT_MODEL=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',SFM64_DIAGNOSTIC_BLEND_TEST='6',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_COUNT='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60)
log=run.stdout+run.stderr;Path('outputs/sfm64-cross-model-gpu-slots.txt').write_text(log)
assert run.returncode==0,log
assert 'cross_model_slots=3 geometry_source=controlled_layers' in log
assert 'GPU blend mode=6' in log
pixels=(directory/'vulkan-controlled-blend-6.rgba').read_bytes();assert len(pixels)==512*512*4
center=list(pixels[(256*512+256)*4:(256*512+256)*4+4]);expected=[128,64,63,255]
assert all(abs(a-b)<=1 for a,b in zip(center,expected))
Path('outputs/sfm64-cross-model-gpu-slots.json').write_text(json.dumps(dict(descriptor_model_slots=3,center_rgba=center,expected_rgba=expected,tolerance=1,scope='controlled triangle layers with cross-model command identities; real multi-asset session integration unfinished'),indent=2)+'\n')
print('cross-model descriptor slots executed on GPU; controlled compositing pixel passed '+str(center))
