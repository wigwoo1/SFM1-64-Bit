from pathlib import Path
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start=image.offset(0x10a31655);end=image.offset(0x10a31ba6)
code=image.b[start:end]
Path('work/original_double_packer.inc').write_text('static const unsigned char originalCode[]={'+','.join(hex(b) for b in code)+'};\nstatic const uint32_t originalConfig[]={'+','.join(hex(image.word(a)) for a in range(0x10f22858,0x10f22870,4))+'};\n')
print(f'Extracted {len(code)} original routine bytes and six conversion constants')
