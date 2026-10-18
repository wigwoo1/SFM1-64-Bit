from pathlib import Path
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
code=image.b[image.offset(0x10a102ce):image.offset(0x10a10408)]
Path('work/original_decimal_crt.inc').write_text('static const unsigned char originalCrt[]={'+','.join(hex(b) for b in code)+'};\n')
print(f'Extracted {len(code)} outer CRT bytes')
