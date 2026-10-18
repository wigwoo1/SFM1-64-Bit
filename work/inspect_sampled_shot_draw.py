from pathlib import Path
from PIL import Image
raw=Path('outputs/mamagen-vulkan-session-camera-v1.rgba').read_bytes()
assert len(raw)==512*512*4
Path('outputs/mamagen-vulkan-sampled-shot-v1.rgba').write_bytes(raw)
Image.frombytes('RGBA',(512,512),raw).save('outputs/mamagen-vulkan-sampled-shot-v1.png')
