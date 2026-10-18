from pathlib import Path
from PIL import Image
root=Path('outputs/saved-movie-output-prefetch-0')
Image.frombytes('RGBA',(3840,2160),(root/'vulkan-retained-frame-1.rgba').read_bytes()).save(root/'four-model-movie-frame.png')
