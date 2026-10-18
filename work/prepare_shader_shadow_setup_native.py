from pathlib import Path
import struct,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\stdshader_dx9.dll')
raw=im.b[im.offset(0x10064ab0):im.offset(0x10064b73)]
assert len(raw)==195 and raw[-1:]==b'\xc3'
patches=[]
for original,target in [(0x100db940,4096),(0x100db944,4100)]:
 needle=struct.pack('<I',original);at=0
 while True:
  at=raw.find(needle,at)
  if at<0:break
  patches.append((at,target));at+=4
assert len(patches)==10,len(patches)
Path('work/shader_shadow_setup_native_bytes.h').write_text('#pragma once\ninline constexpr unsigned char shaderShadowSetupNativeBytes[]={'+','.join(hex(b) for b in raw)+'};\ninline constexpr unsigned shaderShadowSetupPatches[][2]={'+','.join('{'+str(a)+','+str(b)+'}' for a,b in patches)+'};\n')
Path('outputs/sfm64-shader-shadow-setup-native-preparation.json').write_text(json.dumps(dict(sha256=hashlib.sha256(im.b).hexdigest(),bytes=len(raw),patches=patches,limitations='Owned replacement globals and API callbacks. No DLL initialization or live GPU.'),indent=2))
print('native_bytes',len(raw),'global_relocations',len(patches))

