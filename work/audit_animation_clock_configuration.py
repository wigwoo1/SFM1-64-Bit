from pathlib import Path
from pe_image import Image
import re,json
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');lines=[];strings=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=0x1030c880:break
   if va>=0x1030c780:
    lines.append(line);p=re.search(r'push\s+([0-9A-F]{8})h',line)
    if p:
     try:strings.append(dict(instruction=hex(va),text=im.string(int(p[1],16))))
     except (ValueError,UnicodeDecodeError):pass
Path('outputs/sfm64-animation-clock-configuration-registration.txt').write_text(''.join(lines));Path('outputs/sfm64-animation-clock-configuration-audit.json').write_text(json.dumps(strings,indent=2));print(''.join(lines));print(json.dumps(strings,indent=2))
