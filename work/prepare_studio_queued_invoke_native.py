from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll');source=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text());assert hashlib.sha256(im.b).hexdigest()==source['sha256']
a,z=0x10055880,0x1005588f;rows=[r for r in source['instructions'] if a<=int(r['address'],16)<z];covered=set()
for r in rows:
 va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)));assert 'ds:[' not in r['instruction']
 if r['instruction'].startswith('call'):assert b==b'\xff\xd2'
assert covered==set(range(a,z));b=im.b[im.offset(a):im.offset(a)+z-a]
Path('work/studio_queued_invoke_native_init.h').write_text('#pragma once\ninline constexpr unsigned char studioQueuedInvokeNativeBytes[]={'+','.join(hex(x) for x in b)+'};\n');Path('outputs/sfm64-studio-queued-invoke-native-preparation.json').write_text(json.dumps({'studio_sha256':source['sha256'],'bytes':len(b),'checked_instructions':len(rows),'relocations':0},indent=2));print('prepared15 invoke bytes')
