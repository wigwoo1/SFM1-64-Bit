from pe_image import Image
from pathlib import Path
p=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
starts=[p.word(0x10b90f90+0x10),p.word(0x10b90f90+0x14)]
out=[]
for address in starts:
 for i,line in enumerate(lines):
  if line.lstrip().startswith(f'{address:08X}:'):
   out.extend(lines[i:i+90]);break
Path('work/ifm-float-curveinfo-type-chain.txt').write_text('\n'.join(out))
print('\n'.join(out[:90]))


