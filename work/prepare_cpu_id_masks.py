from pathlib import Path
import json,hashlib
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll')
audit=json.loads(Path('outputs/sfm64-cpu-information-export-audit.json').read_text());assert audit['dll_sha256']==hashlib.sha256(image.b).hexdigest()
start,end=0x100030e0,0x10003162;covered={}
for row in audit['verified_instruction_rows']:
 address=int(row['address'],16)
 if start<=address<end:
  for i,value in enumerate(bytes.fromhex(row['bytes'])):covered[address+i]=value
raw=bytes(covered[address] for address in range(start,end));assert raw==image.b[image.offset(start):image.offset(start)+len(raw)]
Path('work/cpu_id_masks_native_init.h').write_text('#pragma once\nstatic const unsigned char nativeCpuMaskBytes[]={'+','.join(hex(x) for x in raw)+'};\nstatic const unsigned nativeCpuMaskOffset='+str(image.offset(start))+';\n')
print('cpu_id_masks_prepared_bytes='+str(len(raw)))
