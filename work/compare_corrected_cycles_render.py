"""Compare owned actual render images; does not establish native Source parity."""
import json
from pathlib import Path
from PIL import Image, ImageChops, ImageStat
root=Path(__file__).resolve().parent.parent
folder=root/'work/cj/b0a3ae89'
evidence=json.loads((folder/'pipeline-verification.json').read_text())
if evidence['render']['state']!='complete':raise ValueError('Render is not complete')
baseline=root/'outputs/cycles-conversion/short-path-session/renders/749d1aad-20fc-4fbf-b1cc-d58c2b12025a/render.png'
corrected=Path(evidence['render']['result']['output_png'])
with Image.open(baseline) as image:a=image.convert('RGB')
with Image.open(corrected) as image:b=image.convert('RGB')
if a.size!=b.size:raise ValueError('Comparison dimensions differ')
difference=ImageChops.difference(a,b)
mean_a,mean_b=ImageStat.Stat(a).mean,ImageStat.Stat(b).mean
weights=(.2126,.7152,.0722)
report=dict(baseline=str(baseline),corrected=str(corrected),dimensions=list(a.size),
    mean_absolute_rgb_error=ImageStat.Stat(difference).mean,
    exact_pixels=difference.getbbox() is None,
    mean_luminance_baseline=sum(x*w for x,w in zip(mean_a,weights)),
    mean_luminance_corrected=sum(x*w for x,w in zip(mean_b,weights)),
    scope='Same converted shot/frame/settings; native Source appearance and other shots unverified.')
(folder/'render-comparison.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report),flush=True)
