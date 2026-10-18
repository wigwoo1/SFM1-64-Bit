from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
r=region(0x100d5eb0,0x100d5fa7);begin=0x100d5eb0;end=0x100d5fa7
opt=im.u32(60)+24;rv,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rv);limit=cursor+size;fix=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and cursor+length<=limit
 for o in range(cursor+8,cursor+length,2):
  e=im.u16(o);assert e>>12 in (0,3)
  a=im.base+page+(e&4095)
  if e>>12==3 and begin<=a<end:fix.append((a-begin,im.word(a)))
 cursor+=length
assert all(v in [0x101526e8,0x10144098,0x10198eb0,0x1019feb0] for _,v in fix)
raw=im.b[im.offset(begin):im.offset(begin)+247]
h='#pragma once\nstatic const unsigned char seedBytes[]={'+','.join(map(str,raw))+'};\nstatic const unsigned seedFileOffset='+str(im.offset(begin))+';\n'
h+='static const unsigned seedFixups[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in fix)+'};\n'
Path('work/seeded_generator_native_init.h').write_text(h)
print('prepared_seed_bytes=247 fixups='+str(len(fix)))
