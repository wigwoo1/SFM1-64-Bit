from pathlib import Path
from fractions import Fraction
import re,json,hashlib
log=Path('outputs/sfm64-native-frame-grid-vulkan-validation.txt').read_text();states=re.findall(r'live_session_frame=(\d+) tick=(\d+) shot=(\d+) rebuilt=(\d+)',log);assert len(states)==40
for n,(frame,tick,shot,rebuilt) in enumerate(states):assert (int(frame),int(tick),int(shot),int(rebuilt))==(n,870833+int(Fraction(n*1000,3)+Fraction(1,2)),2470,int(n==0))
for frame,name in ((0,'mamagen-vulkan-sampled-shot-v1.rgba'),(30,'mamagen-vulkan-sampled-shot-second-tick-v1.rgba')):assert Path(f'outputs/vulkan-retained-frame-{frame}.rgba').read_bytes()==Path('outputs',name).read_bytes(),frame
assert 'model_gpu_presentation_passed=1 frames=40' in log
hashes=[hashlib.sha256(Path(f'outputs/vulkan-retained-frame-{n}.rgba').read_bytes()).hexdigest() for n in range(40)]
Path('outputs/sfm64-native-frame-grid-vulkan-summary.json').write_text(json.dumps({'frames':40,'rate_packed':65536003,'nominal_fps':30,'rounding':'recovered nearest mode 0','last_tick':int(states[-1][1]),'unique_render_hashes':len(set(hashes)),'frame_0_and_30_source_pixels_exact':True,'native_converter_oracle_cases':378,'wall_clock_scheduling':'unfinished','swapchain_visual_pixels':'unverified'},indent=2)+'\n')
print('40 native-converted frame positions verified; frames 0 and 30 pixel-exact; frame 39 offset=13000 ticks')
