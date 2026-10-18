from pathlib import Path
import json
r=json.loads(Path('outputs/sfm64-device-manager-allocation-owner.json').read_text())
import sys
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
lines=['#pragma once']
for key in ['connect','disconnect','allocation_thunk']:
 body=r[key];raw=b''.join(bytes.fromhex(x['bytes']) for x in body['instructions']);assert len(raw)==body['bytes']
 assert im.b[im.offset(int(body['start'],16)):im.offset(int(body['start'],16))+len(raw)]==raw
 lines.append('static const unsigned manager_'+key+'_offset='+str(im.offset(int(body['start'],16)))+';')
 lines.append('static const unsigned char manager_'+key+'[]={'+','.join(hex(x) for x in raw)+'};')
Path('work/device_manager_backend_native_init.h').write_text('\n'.join(lines)+'\n')

