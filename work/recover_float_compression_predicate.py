from pathlib import Path
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
for slot in [0xcc,0xd0]:
 a=im.word(0x10B9202C+slot);i=next(i for i,s in enumerate(lines) if s.startswith(f'  {a:08X}:'));p=lines[i:i+70];Path(f'work/ifm-float-compression-slot-{slot:02X}.txt').write_text('\n'.join(p)+'\n');print(f'slot {slot:02X} -> {a:08X}');print('\n'.join(p[:32]))
