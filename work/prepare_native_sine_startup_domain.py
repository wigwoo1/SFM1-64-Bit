from pathlib import Path
import json,struct,math
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
body=region(0x100e529b,0x100e5453)
classified=[]
for i in range(256):
 angle=float(i)*2.0*struct.unpack_from('<d',im.b,im.offset(0x100fc8c0))[0]*0.00390625
 word=struct.unpack('<4H',struct.pack('<d',angle))[3]&0x7fff
 reduced=(word-0x3030)&0xffff
 if reduced<=0x10c5:path='normal'
 else:
  signed=reduced if reduced<0x8000 else reduced-0x10000
  assert signed<=0
  assert (reduced>>4)==0xcfd
  path='tiny_multiply_return'
 classified.append(dict(index=i,angle=angle,top_word=hex(word),path=path))
assert sum(x['path']=='normal' for x in classified)==255
opt=im.u32(60)+24;rva,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rva);end=cursor+size;relocations=[]
while cursor<end:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and cursor+length<=end
 for p in range(cursor+8,cursor+length,2):
  entry=im.u16(p)
  if entry>>12==3:relocations.append(im.base+page+(entry&4095))
  else:assert entry>>12==0
 cursor+=length
start=int(body['start'],16);limit=int(body['end'],16);fixups=[(a-start,im.word(a)) for a in relocations if start<=a<limit]
assert all(v==0x10159d30 or 0x1015a530<=v<0x1015a5c0 for p,v in fixups)
boundaries={int(x['address'],16) for x in body['instructions']}
for row in body['instructions']:
 a=int(row['address'],16);b=bytes.fromhex(row['bytes'])
 if b[0]==0x0f and 0x80<=b[1]<=0x8f:assert a+6+struct.unpack_from('<i',b,2)[0] in boundaries
 elif b[0]==0xeb or 0x70<=b[0]<=0x7f:assert a+2+struct.unpack_from('<b',b,1)[0] in boundaries
assert [x['target'] for x in body['direct_calls']]==['0x100ec870']
raw=im.b[im.offset(start):im.offset(start)+limit-start]
header='#pragma once\nstatic const unsigned nativeSineFileOffset='+str(im.offset(start))+';\nstatic const unsigned char nativeSineBytes[]={'+','.join(map(str,raw))+'};\nstatic const unsigned nativeSineFixups[][2]={'+','.join('{'+str(p)+','+hex(v)+'}' for p,v in fixups)+'};\n'
blobs=[]
for name,address,count in [('lookup',0x10159d30,2048),('constants',0x1015a530,144)]:
 data=im.b[im.offset(address):im.offset(address)+count];assert len(data)==count
 header+='static const unsigned nativeSine_'+name+'_fileOffset='+str(im.offset(address))+';\nstatic const unsigned char nativeSine_'+name+'[]={'+','.join(map(str,data))+'};\n';blobs.append(dict(name=name,address=hex(address),bytes=count,file_offset=im.offset(address)))
Path('work/native_sine_startup_domain_init.h').write_text(header)
report=dict(sha256=sha,body=body,fixups=fixups,blobs=blobs,startup_angles=classified,finding='All256 startup inputs classified through installed top-word guards:255normal reduction/polynomial table path,zero through tiny multiply-return. None reaches large-angle fallback100ec870. Exact original code/data restricted and extracted for private execution. SSE aligned constant/table buffers must retain16byte alignment.',limits='Offline domain/control proof only; real original sine notexecuted yet. Generalangles, specialvalues/largefallback/MXCSR rounding excluded.')
Path('outputs/sfm64-native-sine-startup-domain.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_native_sine_body_bytes='+str(body['bytes'])+' normal_angles=255 tiny_angles=1 data_bytes=2192')
