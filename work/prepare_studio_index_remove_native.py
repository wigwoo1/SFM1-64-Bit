from pathlib import Path
import json,sys,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
source=json.loads(Path('outputs/sfm64-studio-model-release-dispatch.json').read_text())
assert hashlib.sha256(im.b).hexdigest()==source['sha256']
header=['#pragma once'];counts=[]
for name,a,z in [('48',0x10003fe0,0x1000404f),('24',0x10004050,0x100040bd)]:
 rows=[r for r in source['instructions'] if a<=int(r['address'],16)<z];covered=set()
 for r in rows:
  va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;covered.update(range(va,va+len(b)))
  assert 'call' not in r['instruction'] and 'ds:[' not in r['instruction']
 assert covered==set(range(a,z));code=im.b[im.offset(a):im.offset(a)+z-a]
 header.append('inline constexpr unsigned char studioRemove'+name+'NativeBytes[]={'+','.join(hex(x) for x in code)+'};');counts.append({'stride':int(name),'bytes':len(code),'checked_instructions':len(rows)})
Path('work/studio_index_remove_native_init.h').write_text('\n'.join(header)+'\n')
Path('outputs/sfm64-studio-index-remove-native-preparation.json').write_text(json.dumps({'studio_sha256':source['sha256'],'functions':counts,'relocations':0},indent=2))
print(counts)
