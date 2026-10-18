from pathlib import Path
from PIL import Image
for name in ['glaceon','lotion-cat']:
 p=Path('outputs',name+'-vulkan-diagnostic-v1.rgba');Image.frombytes('RGBA',(512,512),p.read_bytes()).save(p.with_suffix('.png'))
