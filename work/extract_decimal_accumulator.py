from pathlib import Path
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
code=image.b[image.offset(0x10a320f8):image.offset(0x10a322dc)]
Path('work/original_decimal_accumulator.inc').write_text('static const unsigned char originalAccumulator[]={'+','.join(hex(b) for b in code)+'};\n')
print(f'Extracted {len(code)} accumulator bytes')
