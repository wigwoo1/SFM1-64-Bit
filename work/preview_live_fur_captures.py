from pathlib import Path
from PIL import Image
import json
root=Path(__file__).resolve().parent.parent
report=json.loads((root/'outputs/sfm64-live-fur-gpu-validation.json').read_text())
directory=Path(report['output_directory'])
for name in ['baseline','fur']:
 data=(directory/name/'vulkan-retained-frame-1.rgba').read_bytes()
 assert len(data)==512*512*4
 Image.frombytes('RGBA',(512,512),data).save(directory/(name+'.png'))
print(directory/'fur.png')
