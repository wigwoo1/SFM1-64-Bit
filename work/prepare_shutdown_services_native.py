from pathlib import Path
import json,struct,sys
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
r=json.loads(Path('outputs/sfm64-device-manager-base-disconnect.json').read_text())['regions']
lines=['#pragma once']
for name,key,addresses in [('resource','service_cleanup_a',[0x10190904,0x101b11fc,0x101b1200]),('cvar','service_cleanup_b',[0x101908e4,0x1019fcf4,0x1017b7f0])]:
 body=r[key];raw=b''.join(bytes.fromhex(x['bytes']) for x in body['instructions']);off=im.offset(int(body['start'],16));assert im.b[off:off+len(raw)]==raw;fix=[]
 for value in addresses:
  start=0
  while True:
   p=raw.find(struct.pack('<I',value),start)
   if p<0:break
   fix.append((p,value));start=p+4
 lines.extend(['static const unsigned '+name+'Offset='+str(off)+';','static const unsigned char '+name+'Bytes[]={'+','.join(hex(x) for x in raw)+'};','static const unsigned '+name+'Fixups[][2]={'+','.join('{'+str(p)+','+hex(v)+'}' for p,v in fix)+'};'])
Path('work/shutdown_services_native_init.h').write_text('\n'.join(lines)+'\n')
print('verified_shutdown_service_native_bytes=112')
