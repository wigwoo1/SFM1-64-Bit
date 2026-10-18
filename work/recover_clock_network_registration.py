from pe_image import Image
from pathlib import Path
import json
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
rows=[]
for va in range(0x1040ea80,0x1040ead0,4):
 p=im.word(va);r=dict(address=hex(va),value=hex(p))
 try:
  s=im.string(p)
  if s and all(32<=ord(c)<127 for c in s):r['string']=s
 except (ValueError,UnicodeDecodeError):pass
 rows.append(r)
Path('outputs/sfm64-clock-network-registration.json').write_text(json.dumps(rows,indent=2))
print(json.dumps(rows,indent=2))
