from pathlib import Path
from PIL import Image
source=Path('outputs/mamagen-vulkan-session-camera-v1.rgba')
raw=source.read_bytes()
Path('outputs/mamagen-vulkan-posed-session-v1.rgba').write_bytes(raw)
Image.frombytes('RGBA',(512,512),raw).save('outputs/mamagen-vulkan-posed-session-v1.png')
