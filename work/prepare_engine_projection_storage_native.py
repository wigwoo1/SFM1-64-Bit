from pathlib import Path
import hashlib,json,sys
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
assert hashlib.sha256(im.b).hexdigest()=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
report=json.loads(Path('outputs/sfm64-engine-shadow-manager-binding.json').read_text())
routines=[('derived',0x102467e0,0x10246852),('four',0x10246860,0x102468d7),('two',0x102468e0,0x1024695d)]
lines=['#pragma once'];metadata=[]
for name,start,end in routines:
 code=bytearray(im.b[im.offset(start):im.offset(start)+end-start]);fixes=[]
 for row in report['instructions']:
  va=int(row['address'],16)
  if not start<=va<end:continue
  b=bytes.fromhex(row['bytes']);op=row['instruction']
  if 'ds:[' in op:
   target=int(op.split('ds:[')[1].split('h')[0],16)
   assert target in [0x103b6020,0x103b6140]
   assert int.from_bytes(b[-4:],'little')==target
   fixes.append({'offset':va-start+len(b)-4,'kind':'absolute','target':hex(target)})
  if op.startswith('call'):
   assert b[0]==0xe8
   target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target==0x102467e0
   fixes.append({'offset':va-start+1,'kind':'relative','target':hex(target)})
 lines.append('inline constexpr unsigned char '+name+'Bytes[]={'+','.join(hex(v) for v in code)+'};')
 metadata.append({'name':name,'start':hex(start),'size':len(code),'relocations':fixes})
Path('work/engine_projection_storage_native_bytes.h').write_text('\n'.join(lines)+'\n')
Path('outputs/sfm64-engine-projection-storage-native-preparation.json').write_text(json.dumps({'engine_sha256':hashlib.sha256(im.b).hexdigest(),'routines':metadata,'status':'Prepared original bytes and exhaustively identified absolute constant/relative call relocations. Native execution not yet performed.','limitations':'Storage helpers only; full plane generation/live engine omitted.'},indent=2))
for r in metadata:print(r)
