from pathlib import Path
import re,json,hashlib
log=Path('outputs/sfm64-live-tick-grid-vulkan-validation.txt').read_text()
states=re.findall(r'live_session_frame=(\d+) tick=(\d+) shot=(\d+) rebuilt=(\d+)',log)
assert len(states)==40
for n,(frame,tick,shot,rebuilt) in enumerate(states):assert (int(frame),int(tick),int(shot),int(rebuilt))==(n,870833+333*n,2470,int(n==0))
assert 'model_gpu_presentation_passed=1 frames=40' in log
hashes=[]
for n in range(40):
 raw=Path(f'outputs/vulkan-retained-frame-{n}.rgba').read_bytes();assert len(raw)==512*512*4;hashes.append(hashlib.sha256(raw).hexdigest())
assert len(set(hashes))>1
assert Path('outputs/vulkan-retained-frame-0.rgba').read_bytes()==Path('outputs/mamagen-vulkan-sampled-shot-v1.rgba').read_bytes()
Path('outputs/sfm64-live-tick-grid-vulkan-summary.json').write_text(json.dumps({'start_tick':870833,'step_ticks':333,'frames':40,'last_tick':883820,'unique_render_hashes':len(set(hashes)),'prepared_rebuilds':1,'first_frame_source_pixels_exact':True,'ticks_per_second':'unverified','swapchain_visual_pixels':'unverified'},indent=2)+'\n')
print(f'40 live session tick-grid frames presented; {len(set(hashes))} distinct rendered frames; one evaluator preparation')
# Keep the frame-grid variable distinct from the explicit-list cursor.
p=Path('work/vulkan_model_swapchain_probe_candidate.h');s=p.read_text().replace('int32_t start=0,step=0','int32_t gridStart=0,step=0').replace('startText+startSize,start);','startText+startSize,gridStart);').replace('sessionTickGridCandidate(start,step,count)','sessionTickGridCandidate(gridStart,step,count)');p.write_text(s)
