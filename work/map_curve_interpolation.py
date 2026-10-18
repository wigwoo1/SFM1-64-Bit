from pathlib import Path
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
lines=Path('work/ifm-curve-interpolation-dispatch.txt').read_text().splitlines()
for code in range(16):
 addr=im.word(0x10066CF8+4*code)
 start=next(i for i,s in enumerate(lines) if s.startswith(f'  {addr:08X}:'))
 print(f'CODE {code} -> {addr:08X}')
 print('\n'.join(lines[start:start+22]))
