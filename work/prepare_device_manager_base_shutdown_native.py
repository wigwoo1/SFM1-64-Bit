from pathlib import Path
import json,sys
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
r=json.loads(Path('outputs/sfm64-device-manager-base-disconnect.json').read_text())['regions']['base_disconnect']
raw=b''.join(bytes.fromhex(x['bytes']) for x in r['instructions']);off=im.offset(int(r['start'],16));assert im.b[off:off+len(raw)]==raw
Path('work/device_manager_base_shutdown_native_init.h').write_text('#pragma once\nstatic const unsigned shutdownOffset='+str(off)+';\nstatic const unsigned char shutdownBytes[]={'+','.join(hex(x) for x in raw)+'};\n')
