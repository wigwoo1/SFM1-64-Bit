from pathlib import Path
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
code=image.b[image.offset(0x10a1ded7):image.offset(0x10a1e5b3)]
Path('work/original_decimal_parser.inc').write_text('static const unsigned char originalParser[]={'+','.join(hex(b) for b in code)+'};\n')
print(f'Extracted {len(code)} parser/dispatch bytes')
