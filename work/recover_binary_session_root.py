import re,json,hashlib
from pathlib import Path
from pe_image import Image
path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
im=Image(path)
assert hashlib.sha256(im.b).hexdigest()=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
rows=[];pending=None
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'^\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)',line)
 if m:
  va=int(m[1],16);pending=None
  if 0x10395b60<=va<0x10396150:
   pending={'address':hex(va),'bytes':m[2].strip(),'instruction':m[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+\n?',line):pending['bytes']+=' '+line.strip()
for row in rows:
 b=bytes.fromhex(row['bytes']);o=im.offset(int(row['address'],16));assert im.b[o:o+len(b)]==b
Path('outputs/sfm64-binary-session-root-instructions.json').write_text(json.dumps({'sha256':hashlib.sha256(im.b).hexdigest(),'checked_instructions':len(rows),'instructions':rows},indent=2))
Path('outputs/sfm64-binary-session-root-instructions.txt').write_text('\n'.join(f"{r['address']} {r['instruction']}" for r in rows))
print('checked_instructions',len(rows))
