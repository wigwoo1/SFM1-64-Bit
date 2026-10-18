import sys,struct,json,hashlib
from pathlib import Path
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
recovery=json.loads(Path('outputs/sfm64-shader-vertex-layout-recovery.json').read_text())
assert hashlib.sha256(im.b).hexdigest()==recovery['sha256']
a,z=0x10004390,0x10004c50
blob=im.b[im.offset(a):im.offset(a)+z-a]
p=im.u32(60)+24;rva,length=struct.unpack_from('<II',im.b,p+96+5*8)
o=im.offset(im.base+rva);end=o+length;fix=[]
while o<end:
 page,size=struct.unpack_from('<II',im.b,o)
 if not size:break
 for q in range(o+8,o+size,2):
  entry=im.u16(q);va=im.base+page+(entry&4095)
  if entry>>12==3 and a<=va<z:
   target=im.word(va)
   if a<=target<z:offset=target-a
   else:offset={0x1017c370:8192,0x1017f700:8196,0x1017f640:8208}[target]
   fix.append((va-a,offset))
 o+=size
helper=im.b[im.offset(0x100dfcb0):im.offset(0x100dfcb0)+31]
s='#pragma once\nstatic const unsigned char layoutBytes[]={'+','.join(map(str,blob))+'};\nstatic const unsigned char shiftBytes[]={'+','.join(map(str,helper))+'};\n'
s+='inline void initializeLayout(unsigned char* code){std::memcpy(code,layoutBytes,sizeof(layoutBytes));std::memcpy(code+6144,shiftBytes,sizeof(shiftBytes));code[6200]=0xc3;\n'
for at,to in fix:s+=f'{{uint32_t v=uint32_t(reinterpret_cast<uintptr_t>(code+{to}));std::memcpy(code+{at},&v,4);}}\n'
for va,to in [(0x1000465d,6144),(0x100048b6,6200)]:s+=f'{{int32_t v={to}-{va-a+5};std::memcpy(code+{va-a+1},&v,4);}}\n'
s+='}\n';Path('work/shader_vertex_layout_native_init.h').write_text(s)
print('native_bytes',len(blob),'absolute_relocations',len(fix))
