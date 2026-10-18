from pathlib import Path
import json,hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin,end=0x10021087,0x1002114b;covered={}
audit=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text())
for row in audit['instructions']:
 a=int(row['address'],16);raw=bytes.fromhex(row['bytes'])
 if begin<=a<end:
  assert im.b[im.offset(a):im.offset(a)+len(raw)]==raw
  for i,b in enumerate(raw):covered[a+i]=b
assert set(covered)==set(range(begin,end))
raw=bytes(covered[a] for a in range(begin,end))
Path('outputs/restore-dispatch-verified-bytes.bin').write_bytes(raw)
Path('work/restore_dispatch_native_init.h').write_text('#pragma once\nstatic const unsigned restoreDispatchByteCount='+str(len(raw))+';\nstatic const unsigned restoreDispatchFileOffset='+str(im.offset(begin))+';\n')
Path('outputs/sfm64-restore-dispatch-native-preparation.json').write_text(json.dumps({'sha256':sha,'begin':hex(begin),'end':hex(end),'bytes':len(raw),'scope':'Two suppression/list dispatch/clear regions extracted from full derived restore; fixture supplies EBX owner and exit return at span end. Not standalone original function or full restore execution.','bindings':'Owner vtable36 suppression; two owned list pointers/counts and cdecl fourargument callbacks; no direct calls or absolute operands in selected instructions.','limits':'Native execution pending; fixture must preserve EBX/ESI/EDI and provide owner-sized storage.'},indent=2)+'\n')
print(f'prepared_restore_dispatch_bytes={len(raw)}')

