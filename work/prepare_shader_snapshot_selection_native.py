from pathlib import Path
import struct,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
raw=im.b[im.offset(0x100653c0):im.offset(0x1006549b)]
assert len(raw)==219 and raw[-3:]==b'\xc2\x08\x00'
patches=[]
for original,target in [(0x100db940,4096),(0x100db948,4100)]:
 needle=struct.pack('<I',original);at=0
 while True:
  at=raw.find(needle,at)
  if at<0:break
  patches.append((at,target));at+=4
assert len(patches)==6,len(patches)
Path('work/shader_snapshot_selection_native_bytes.h').write_text('#pragma once\ninline constexpr unsigned char shaderSnapshotSelectionNativeBytes[]={'+','.join(hex(b) for b in raw)+'};\ninline constexpr unsigned shaderSnapshotSelectionPatches[][2]={'+','.join('{'+str(a)+','+str(b)+'}' for a,b in patches)+'};\n')
Path('outputs/sfm64-shader-snapshot-selection-native-preparation.json').write_text(json.dumps(dict(sha256=hashlib.sha256(im.b).hexdigest(),bytes=len(raw),patches=patches,limitations='Owned replacement globals and API callbacks. No DLL initialization or live GPU.'),indent=2))
print('native_bytes',len(raw),'global_relocations',len(patches))
