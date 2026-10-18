from pathlib import Path
from fractions import Fraction
import re,json
log=Path('outputs/sfm64-session-rate-grid-vulkan-validation.txt').read_text()
assert 'session_rate_source=renderSettings fps=60 packed=655360060 owner=2232' in log
states=re.findall(r'live_session_frame=(\d+) tick=(\d+) shot=(\d+) rebuilt=(\d+)',log)
assert len(states)==61
for n,(frame,tick,shot,rebuilt) in enumerate(states):
 assert (int(frame),int(tick),int(shot),int(rebuilt))==(n,870833+int(Fraction(n*10000,60)+Fraction(1,2)),2470,int(n==0))
for frame,name in ((0,'mamagen-vulkan-sampled-shot-v1.rgba'),(60,'mamagen-vulkan-sampled-shot-second-tick-v1.rgba')):
 assert Path(f'outputs/vulkan-retained-frame-{frame}.rgba').read_bytes()==Path('outputs',name).read_bytes()
assert 'model_gpu_presentation_passed=1 frames=61' in log
Path('outputs/sfm64-session-rate-grid-vulkan-summary.json').write_text(json.dumps({'saved_rate_fps':60,'packed_rate':655360060,'rate_source':'session settings/renderSettings/frameRate','frame_positions_verified':61,'frame_0_and_60_source_pixels_exact':True,'native_settings_precedence':'unverified','visible_playback':'unverified'},indent=2)+'\n')
print('Saved 60 fps session rate: 61 tick positions verified; frames 0 and 60 pixel-exact')
