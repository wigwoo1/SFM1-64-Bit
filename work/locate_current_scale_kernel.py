from pathlib import Path
import hashlib,json,struct
p=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs\datamodel_pyd.pyd');data=p.read_bytes();old=Path('work/native_local_scale_kernel.inc')
pattern=bytes.fromhex('F3 0F 10 0E F3 0F 59 C8 F3 0F 11 0E');hits=[];cursor=0
while True:
 off=data.find(pattern,cursor)
 if off<0:break
 cursor=off+1
 kernel=data[off:off+124]
 if kernel[-5:]==bytes.fromhex('F3 0F 11 4E 28'):hits.append((off,kernel))
pe=struct.unpack_from('<I',data,60)[0];base=struct.unpack_from('<I',data,pe+52)[0];table=pe+24+struct.unpack_from('<H',data,pe+20)[0];locations=[]
for off,kernel in hits:
 address=None
 for n in range(struct.unpack_from('<H',data,pe+6)[0]):
  o=table+n*40;size,rva,rawsize,raw=struct.unpack_from('<IIII',data,o+8)
  if raw<=off<raw+rawsize:address=base+rva+off-raw
 locations.append(dict(file_offset=off,address=hex(address) if address else None))
result=dict(binary=str(p),sha256=hashlib.sha256(data).hexdigest(),matching_kernel_locations=locations)
Path('outputs/sfm64-current-scale-kernel-locations.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if len(hits)!=1:raise RuntimeError('Expected unique validated arithmetic kernel; extraction refused')
old.write_text(','.join(f'0x{x:02x}' for x in hits[0][1]+b'\xc3'))
