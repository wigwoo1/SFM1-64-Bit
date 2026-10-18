from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll');report=json.loads(Path('outputs/sfm64-material-call-queue-recovery.json').read_text());assert hashlib.sha256(im.b).hexdigest()==report['materialsystem_sha256']
a,z=0x100219e0,0x10021ca6;code=im.b[im.offset(a):im.offset(a)+z-a];fixes=[];covered=set()
for row in report['instructions']:
 va=int(row['address'],16)
 if not a<=va<z:continue
 b=bytes.fromhex(row['bytes']);assert code[va-a:va-a+len(b)]==b;covered.update(range(va,va+len(b)))
 if b[0]==0xe8:
  target=va+5+int.from_bytes(b[1:],'little',signed=True);assert target in (0x10021060,0x1007d8df);fixes.append((va-a+1,'copy' if target==0x10021060 else 'security'))
assert covered==set(range(a,z));assert code[29:33]==(0x10104200).to_bytes(4,'little')
s='#pragma once\ninline constexpr unsigned char indexUploadNativeBytes[]={'+','.join(hex(x) for x in code)+'};\ninline void initializeIndexUploadNative(unsigned char* code,uint32_t copy,uint32_t security){std::memcpy(code,indexUploadNativeBytes,sizeof(indexUploadNativeBytes));uint32_t cookie=reinterpret_cast<uint32_t>(code+4096);std::memcpy(code+29,&cookie,4);\n'
for at,target in fixes:s+='uint32_t displacement'+str(at)+'='+target+'-reinterpret_cast<uint32_t>(code+'+str(at+4)+');std::memcpy(code+'+str(at)+',&displacement'+str(at)+',4);\n'
s+='}\n';Path('work/material_index_upload_native_init.h').write_text(s);print('prepared710 native upload bytes')

