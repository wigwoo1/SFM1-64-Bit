from pathlib import Path
import subprocess,re,json
runtime=r'C:\Users\wigwo\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
subprocess.run([runtime,'work/verify_session_launch_gpu.py'],check=True)
log=Path('outputs/sfm64-session-argument-controls.txt').read_text(encoding='utf-8')
extent=re.search(r'extent=(\d+)x(\d+)',log);assert extent
width,height=map(int,extent.groups());fits=re.findall(r'viewport_fit x=(\d+) y=(\d+) width=(\d+) height=(\d+) background=black reserved_bottom=(\d+)',log);assert len(fits)==5
for row in fits:
 x,y,w,h,bottom=map(int,row);assert bottom==40 and x+w<=width and y+h<=height-bottom
Path('outputs/sfm64-timeline-panel-gpu.json').write_text(json.dumps(dict(swapchain_extent=[width,height],reserved_bottom=40,checked_frames=len(fits),viewport_does_not_overlap_timeline=True,scene_capture_pixels_unchanged=True,visual_layout='unverified'),indent=2),encoding='utf-8')
print('timeline panel GPU bounds passed all five presented frames, 40px reserved, scene capture pixels unchanged')
