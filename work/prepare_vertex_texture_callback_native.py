"""Prepare verified original callback bytes and exhaustive PE fixup inventory."""
from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert hashlib.sha256(im.b).hexdigest()=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
start,end=0x1001f8a0,0x1001fb8f
raw=im.b[im.offset(start):im.offset(start)+end-start]
opt=im.u32(60)+24;rva=im.u32(opt+96+5*8);size=im.u32(opt+96+5*8+4)
cursor=im.offset(im.base+rva);limit=cursor+size;fixups=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor)
 assert length>=8 and length%2==0 and cursor+length<=limit
 for at in range(cursor+8,cursor+length,2):
  entry=im.u16(at)
  if entry>>12!=3:continue
  address=im.base+page+(entry&4095)
  if start<=address<end:
   assert address+4<=end;fixups.append((address-start,im.word(address)))
 cursor+=length
assert fixups==[(5,0x1019092c)]
calls=[];indirect=[]
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if not m:continue
 address=int(m[1],16)
 if not start<=address<end:continue
 prefix=bytes.fromhex(m[2]);assert raw[address-start:address-start+len(prefix)]==prefix
 text=m[3].strip()
 if text.startswith('call'):
  if prefix[0]==0xe8:
   assert len(prefix)==5
   target=address+5+struct.unpack('<i',prefix[1:])[0]
   assert target==0x10002da0;calls.append((address-start,target))
  else:indirect.append({'address':hex(address),'instruction':text})
assert len(calls)==6 and len(indirect)==7
header='#pragma once\nstatic const unsigned char vertexTextureCallbackBytes[]={'+','.join(map(str,raw))+'};\n'
header+=f'static const unsigned vertexTextureCallbackFileOffset={im.offset(start)};\n'
for name,values in [('Fixups',fixups),('Calls',calls)]:
 header+='static const unsigned vertexTextureCallback'+name+'[][2]={'+','.join('{'+str(a)+','+str(b)+'}' for a,b in values)+'};\n'
Path('work/vertex_texture_callback_native_init.h').write_text(header)
report={'source_sha256':hashlib.sha256(im.b).hexdigest(),'body_start':hex(start),'body_bytes':len(raw),'absolute_fixups':fixups,'direct_calls':calls,'indirect_calls':indirect,
 'required_bindings':'Private service pointer with slot31 signed count; command device slots65/69; refill controlled or trapped. Preserve native current/cache24-byte layout only inside x86 oracle.',
 'comparison_scope':'Full command stream, cache record and sampler fields, COM arguments and order; include force low-byte, equality masks, null records, direct/array resource selection and callback field mutation.',
 'limits':'Preparation only. Native execution not yet performed; negative record-pointer sentinel domain and GPU/refill/service lifetime remain unresolved.'}
Path('outputs/sfm64-vertex-texture-callback-native-preparation.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'prepared_callback_bytes={len(raw)} absolute_fixups={len(fixups)} refill_calls={len(calls)} indirect_calls={len(indirect)}')
