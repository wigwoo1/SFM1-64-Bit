from pathlib import Path
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
for a,n in [(0x102BDBD0,90),(0x1038BA20,95)]:
 i=next(i for i,s in enumerate(lines) if s.startswith(f'  {a:08X}:'));p=lines[i:i+n];Path(f'work/ifm-float-compressed-helper-{a:08X}.txt').write_text('\n'.join(p)+'\n');print('\n'.join(p[:48]))
try:print('fallback disk value',hex(im.word(0x10F60300)))
except Exception as e:print('fallback no mapped disk bytes',str(e))
