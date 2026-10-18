from pathlib import Path
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
b=image.b[image.offset(0x10a0f924):image.offset(0x10a0f932)]
Path('work/original_modulo.inc').write_text('static const unsigned char originalModulo[]={'+','.join(hex(x) for x in b)+'};\n')
