"""Inventory relocation requirements; this does not execute native code."""
from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x100167b0,0x10016a78
raw=im.b[im.offset(start):im.offset(start)+end-start]
recovery=json.loads(Path('outputs/sfm64-shader-mode-tail-recovery.json').read_text())
assert recovery['installed_sha256']==hashlib.sha256(im.b).hexdigest()
assert recovery['verified_bytes']==len(raw)
optional=im.u32(60)+24
rva,size=struct.unpack_from('<II',im.b,optional+96+40)
cursor=im.offset(im.base+rva);limit=cursor+size;fixups=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor)
 if not length:break
 assert length>=8 and cursor+length<=limit
 for offset in range(cursor+8,cursor+length,2):
  entry=im.u16(offset);address=im.base+page+(entry&4095)
  if entry>>12==3 and start<=address<end:
   assert address+4<=end
   fixups.append({'offset':address-start,'target':im.word(address)})
 cursor+=length
calls=[];indirect=[]
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):.*?\bcall\s+(.+)',line)
 if not m:continue
 address=int(m[1],16)
 if not start<=address<end:continue
 operand=m[2].strip()
 if re.fullmatch('[0-9A-F]{8}',operand):
  offset=address-start;target=int(operand,16)
  assert raw[offset]==0xe8 and address+5+struct.unpack_from('<i',raw,offset+1)[0]==target
  calls.append({'offset':offset,'target':target})
 else:indirect.append({'address':hex(address),'operand':operand})
assert {f['target'] for f in fixups}=={0x1017c370,0x100fa5f8,0x100fc898,0x1016d130}
assert {c['target'] for c in calls}=={0x100e01aa,0x100df5ad,0x10002da0,0x100df030}
header='#pragma once\nstatic constexpr unsigned char nativePixelColorBytes[]={'+','.join(map(str,raw))+'};\n'
header+='static constexpr unsigned nativePixelColorFileOffset='+str(im.offset(start))+';\n'
for name,entries in [('Fixups',fixups),('Calls',calls)]:
 header+='static constexpr unsigned nativePixelColor'+name+'[][2]={'+','.join('{'+str(e['offset'])+','+str(e['target'])+'}' for e in entries)+'};\n'
Path('work/pixel_color_native_init.h').write_text(header)
report={'installed_sha256':recovery['installed_sha256'],'body_bytes':len(raw),'absolute_fixups':fixups,'direct_calls':calls,'indirect_calls':indirect,
 'required_bindings':['security cookie and check','isolated original double math helper','cdecl memcpy','controlled queue flush or prove direct-only branch','fake secondary/global/device vtables with exact x86 calling conventions'],
 'limitations':'Preparation only; no execution. All listed references require remapping before invocation. Full owner storage and writable cache capacity must be provided.'}
Path('outputs/sfm64-pixel-color-native-preparation.json').write_text(json.dumps(report,indent=2))
print('pixel_color_bytes='+str(len(raw))+' absolute_fixups='+str(len(fixups))+' direct_calls='+str(len(calls))+' indirect_calls='+str(len(indirect)))
