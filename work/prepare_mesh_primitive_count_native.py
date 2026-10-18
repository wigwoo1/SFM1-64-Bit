from pathlib import Path
from pe_image import Image
import json,hashlib
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x10006d20,0x10006d94
raw=im.b[im.offset(start):im.offset(end)]
assert len(raw)==116 and im.word(start+15)==0x10006d7c
targets=[im.word(0x10006d7c+4*i)-start for i in range(6)]
assert targets==[19,26,83,38,57,67]
Path('work/mesh_primitive_count_native_bytes.h').write_text('#pragma once\ninline constexpr unsigned char meshPrimitiveCountNativeBytes[]={'+','.join(hex(b) for b in raw)+'};\ninline constexpr unsigned meshPrimitiveCountTargets[]={'+','.join(str(t) for t in targets)+'};\n')
Path('outputs/sfm64-mesh-primitive-count-native-preparation.json').write_text(json.dumps(dict(sha256=hashlib.sha256(im.b).hexdigest(),copied_bytes=len(raw),jump_table_offset=92,table_reference_offset=15,target_offsets=targets,limitations='Original execution requiresx86; owned meshfixture; no GPU.'),indent=2))
print('primitive_count_native_bytes',len(raw))
