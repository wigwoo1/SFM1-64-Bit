from pathlib import Path
from PIL import Image
import json,statistics,re
root=Path('outputs/animated-four-model-scene')
for n in (0,30,60):
 Image.frombytes('RGBA',(512,512),(root/f'vulkan-retained-frame-{n}.rgba').read_bytes()).save(root/f'frame-{n}.png')
profiles={}
for name in ('sfm64-animated-four-model-gpu.txt','sfm64-animated-four-model-gpu-serial.txt'):
 text=Path('outputs',name).read_text()
 times=[float(t) for t in re.findall(r'cpu_prepare_ms=([0-9.]+)',text)]
 profiles[name]=dict(samples=len(times),median_cpu_prepare_ms=statistics.median(times[1:]),scope='capture enabled, sequential runs, includes scene evaluation and vertex upload; not an isolated speedup benchmark')
Path('outputs/sfm64-animated-four-model-timing-observations.json').write_text(json.dumps(profiles,indent=2)+'\n')
print(json.dumps(profiles))
