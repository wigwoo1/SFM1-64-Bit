from pathlib import Path
import json
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
report=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text())
header='#pragma once\n'
for name,start,end in [('First',0x1002aaa0,0x1002aad1),('Second',0x1002abe0,0x1002ac61)]:
 rows=[r for r in report['instructions'] if start<=int(r['address'],16)<end]
 raw=b''.join(bytes.fromhex(r['bytes']) for r in rows)
 assert len(raw)==end-start and raw==im.b[im.offset(start):im.offset(start)+len(raw)]
 assert all(not r['instruction'].startswith('call') or r['instruction']=='call        edx' for r in rows)
 header+='static const unsigned char ownerResource'+name+'Bytes[]={'+','.join(map(str,raw))+'};\n'
 header+='static const unsigned ownerResource'+name+'Offset='+str(im.offset(start))+';\n'
Path('work/owner_resource_cleanup_native_init.h').write_text(header)
print('resource_cleanup_prepared_bytes=178')
