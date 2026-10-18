"""Generate an isolated-execution inventory from the verified setter body."""
from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
start,end=0x10026fb0,0x1002703f;raw=im.b[im.offset(start):im.offset(start)+end-start]
opt=im.u32(60)+24;cursor=im.offset(im.base+im.u32(opt+136));limit=cursor+im.u32(opt+140);fixups=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and cursor+length<=limit
 for at in range(cursor+8,cursor+length,2):
  entry=im.u16(at);address=im.base+page+(entry&4095)
  if entry>>12==3 and start<=address<end:
   assert address+4<=end;fixups.append((address-start,im.word(address)))
 cursor+=length
assert len(fixups)==4 and {target for _,target in fixups}=={0x1016d130,0x100fd424,0x100fa244,0x10011610}
calls=[];indirect=[]
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if not m:continue
 a=int(m[1],16)
 if not start<=a<end:continue
 prefix=bytes.fromhex(m[2]);assert raw[a-start:a-start+len(prefix)]==prefix
 if not m[3].strip().startswith('call'):continue
 if prefix[0]==0xe8:
  target=a+5+struct.unpack('<i',prefix[1:])[0];assert target in {0x100df030,0x10026890};calls.append((a-start,target))
 else:indirect.append(hex(a))
assert len(calls)==2 and len(indirect)==1
header='#pragma once\nstatic const unsigned char extendedVertexRegistrationBytes[]={'+','.join(map(str,raw))+'};\n'
header+=f'static const unsigned extendedVertexRegistrationFileOffset={im.offset(start)};\n'
for name,items in [('Fixups',fixups),('Calls',calls)]:header+='static const unsigned extendedVertexRegistration'+name+'[][2]={'+','.join('{'+str(a)+','+str(b)+'}' for a,b in items)+'};\n'
Path('work/extended_vertex_constant_registration_native_init.h').write_text(header)
Path('outputs/sfm64-extended-vertex-registration-native-preparation.json').write_text(json.dumps({'sha256':sha,'bytes':len(raw),'fixups':fixups,'calls':calls,'indirect_calls':indirect,
 'bindings':'Private capability-pointer cell, warning format and warning-function import cell, remapped callback pointer; controlled copy and callback append.',
 'comparison':'Entire1536-register staging buffer plus canary, warning/append event order and registration flag; capabilities below/equal/above threshold, zero/clipped ranges, already-registered and callback flag mutation.',
 'limits':'Original execution pending. Owned invalid-input rejection and registration allocation-failure policy are explicit x64 differences; compare supported nonnegative ranges separately.'},indent=2)+'\n')
print(f'prepared_extended_setter_bytes={len(raw)} fixups={fixups} calls={calls}')
