from pathlib import Path
import subprocess,os,json,hashlib
ticks=Path('work/timeline-blank-gpu-ticks.txt');ticks.write_text('870833\n625333\n0\n870833\n')
clear=bytes([13,13,20,255])*(512*512);reference=Path('outputs/material-overrides-prefetch-0/vulkan-retained-frame-0.rgba').read_bytes();runs=[]
for mode in (0,1):
 directory=Path(f'outputs/timeline-blanks-prefetch-{mode}');directory.mkdir(exist_ok=True)
 env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
 env.update(SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='870833',SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),SFM64_DIAGNOSTIC_TIMELINE_BLANKS='1',SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(mode),SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()))
 run=subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],env=env,capture_output=True,text=True,timeout=60);log=run.stdout+run.stderr;Path(f'outputs/sfm64-timeline-blanks-prefetch-{mode}.txt').write_text(log,encoding='utf-8');assert run.returncode==0,log
 pixels=[(directory/f'vulkan-retained-frame-{i}.rgba').read_bytes() for i in range(4)]
 assert pixels[0]==reference and pixels[3]==reference,'scene restore pixels changed'
 assert pixels[1]==clear and pixels[2]==clear,'gap or scene-less frame retained geometry'
 assert 'timeline_frame_state=0 tick=625333' in log and 'timeline_frame_state=1 tick=0' in log
 runs.append(dict(prefetch=mode,sha256=[hashlib.sha256(p).hexdigest() for p in pixels],gap_clear_exact=True,scene_less_clear_exact=True,restored_scene_exact=True))
Path('outputs/sfm64-timeline-blank-gpu.json').write_text(json.dumps(runs,indent=2),encoding='utf-8');print('timeline blanks GPU passed scene -> gap -> scene-less -> scene, both prefetch modes pixels exact; native blank policy unverified')
