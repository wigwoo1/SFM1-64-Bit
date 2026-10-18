from pathlib import Path
import json,hashlib
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll')
audit=json.loads(Path('outputs/sfm64-cpu-information-export-audit.json').read_text())
assert audit['dll_sha256']==hashlib.sha256(image.b).hexdigest()
start,end=0x100036b0,0x10003746
raw=image.b[image.offset(start):image.offset(start)+end-start]
covered={}
for row in audit['verified_instruction_rows']:
 address=int(row['address'],16)
 if start<=address<end:
  for i,value in enumerate(bytes.fromhex(row['bytes'])):covered[address+i]=value
assert raw==bytes(covered[address] for address in range(start,end))
assert raw[6:11]==bytes.fromhex('a1a0a80310') and raw[141]==0xe8
assert int.from_bytes(raw[142:146],'little',signed=True)+start+146==0x1001b3a4
Path('work/cpu_id_count_native_init.h').write_text('#pragma once\nstatic const unsigned char nativeCpuIdBytes[]={'+','.join(hex(x) for x in raw)+'};\nstatic const unsigned nativeCpuIdOffset='+str(image.offset(start))+';\n')
print('cpu_id_count_native_prepared_bytes='+str(len(raw)))
