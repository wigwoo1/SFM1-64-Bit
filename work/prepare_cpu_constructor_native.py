"""Enumerate every PE relocation and disassembled call in the CPU constructor."""
from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll')
start,end=0x100032a0,0x100035d1
audit=json.loads(Path('outputs/sfm64-cpu-information-export-audit.json').read_text())
assert audit['dll_sha256']==hashlib.sha256(im.b).hexdigest()
assert {'start':hex(start),'end':hex(end),'bytes':end-start} in audit['verified_regions']
raw=im.b[im.offset(start):im.offset(start)+end-start]
optional=im.u32(60)+24;rva,size=struct.unpack_from('<II',im.b,optional+136)
cursor=im.offset(im.base+rva);limit=cursor+size;fixups=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and length%2==0 and cursor+length<=limit
 for offset in range(cursor+8,cursor+length,2):
  entry=im.u16(offset);address=im.base+page+(entry&4095)
  if start<=address<end and entry>>12:
   assert entry>>12==3 and address+4<=end
   fixups.append({'offset':address-start,'target':im.word(address)})
 cursor+=length
calls=[];indirect=[]
for row in audit['verified_instruction_rows']:
 address=int(row['address'],16)
 if not start<=address<end:continue
 instruction=row['instruction'];match=re.fullmatch(r'call\s+([0-9A-F]{8})',instruction)
 if match:
  offset=address-start;target=int(match[1],16)
  assert raw[offset]==0xe8 and address+5+struct.unpack_from('<i',raw,offset+1)[0]==target
  calls.append({'offset':offset,'target':target})
 elif instruction.startswith('call'):indirect.append({'address':hex(address),'instruction':instruction})
assert {call['target'] for call in calls}=={0x10002ec0,0x100030e0}
assert len(indirect)==10
header='#pragma once\nstatic constexpr unsigned char nativeCpuConstructorBytes[]={'+','.join(map(str,raw))+'};\n'
header+='static constexpr unsigned nativeCpuConstructorFileOffset='+str(im.offset(start))+';\n'
for name,entries in [('Fixups',fixups),('Calls',calls)]:
 header+='static constexpr unsigned nativeCpuConstructor'+name+'[][2]={'+','.join('{'+str(entry['offset'])+','+str(entry['target'])+'}' for entry in entries)+'};\n'
Path('work/cpu_constructor_native_init.h').write_text(header)
report={'installed_sha256':audit['dll_sha256'],'body_bytes':len(raw),'absolute_fixups':fixups,'direct_calls':calls,'indirect_calls':indirect,'required_bindings':['private cached CPUID globals and vendor storage','private vtable identity','controlled CPUID wrapper10002ec0','verified mask helper100030e0','controlled current process/thread, query/set affinity and Sleep import cells'],'native_executed':False,'limits':'Preparation only. All targets must be explicitly remapped before native execution; private test must never fall through to live affinity imports.'}
Path('outputs/sfm64-cpu-constructor-native-preparation.json').write_text(json.dumps(report,indent=2)+'\n')
print('cpu_constructor_bytes='+str(len(raw))+' fixups='+str(len(fixups))+' direct_calls='+str(len(calls))+' indirect_calls='+str(len(indirect)))
