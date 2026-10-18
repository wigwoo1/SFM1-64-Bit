from pathlib import Path
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
code=image.b[image.offset(0x10a1dd78):image.offset(0x10a1de1f)]
Path('work/original_decimal_conversion.inc').write_text('static const unsigned char originalConversion[]={'+','.join(hex(b) for b in code)+'};\n')
print(f'Extracted {len(code)} conversion-wrapper bytes')
