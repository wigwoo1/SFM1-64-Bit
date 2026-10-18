from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');sha=hashlib.sha256(im.b).hexdigest();assert sha=='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
a,z=0x100d4920,0x100d4972;coverage=set();count=0
for r in json.loads(Path('outputs/sfm64-engine-shadow-manager-binding.json').read_text())['instructions']:
 va=int(r['address'],16)
 if not a<=va<z:continue
 b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b
 assert 'ds:[' not in r['instruction'] and not r['instruction'].startswith('call')
 coverage.update(range(va,va+len(b)));count+=1
assert coverage==set(range(a,z))
code=im.b[im.offset(a):im.offset(a)+z-a]
Path('work/engine_model_render_assignment_native_bytes.h').write_text('#pragma once\ninline constexpr unsigned char modelRenderAssignmentNativeBytes[]={'+','.join(hex(v) for v in code)+'};\n')
Path('outputs/sfm64-engine-model-render-assignment-native-preparation.json').write_text(json.dumps({'engine_sha256':sha,'bytes':len(code),'instructions':count,'relocations':0,'status':'Complete byte coverage; execution pending'},indent=2))
