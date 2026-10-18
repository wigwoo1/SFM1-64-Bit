from pathlib import Path
import json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
records=json.loads(Path('outputs/sfm64-native-scale-string-locations.json').read_text())['locations'];out=[]
for record in records:
 path=Path(record['binary'])
 if path.name not in ('datamodel_pyd.pyd','client.dll'):continue
 data=path.read_bytes();pe=struct.unpack_from('<I',data,60)[0];count=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0];base=struct.unpack_from('<I',data,pe+52)[0];table=pe+24+optional
 sections=[]
 for n in range(count):
  off=table+40*n;size,rva,rawsize,raw=struct.unpack_from('<IIII',data,off+8);flags=struct.unpack_from('<I',data,off+36)[0];sections.append((raw,rawsize,rva,flags))
 dis=Cs(CS_ARCH_X86,CS_MODE_32)
 for match in record['absolute_address_byte_matches']:
  section=next((s for s in sections if s[0]<=match<s[0]+s[1] and s[3]&0x20000000),None)
  if section is None:continue
  # Decode from multiple nearby starts and require an instruction operand
  # containing the entire four-byte string address. Context is heuristic.
  chosen=None
  for start in range(max(section[0],match-40),match+1):
   instructions=list(dis.disasm(data[start:match+100],base+section[2]+start-section[0]))
   target=next((i for i in instructions if i.address-base-section[2]+section[0]<=match and i.address-base-section[2]+section[0]+i.size>=match+4),None)
   if target and hex(record['address']) in target.op_str:
    chosen=instructions;break
  if chosen:
   out.append(dict(binary=str(path),string_address=hex(record['address']),reference_offset=match,context=[f'{i.address:08x}: {i.mnemonic} {i.op_str}' for i in chosen],interpretation='candidate instruction reference; function boundaries and scale application unverified'))
Path('outputs/sfm64-native-scale-reference-disassembly.json').write_text(json.dumps(out,indent=2)+'\n')
print('Executable-section references decoded:',len(out))
for record in out:print(Path(record['binary']).name,record['reference_offset'],'\n'+'\n'.join(record['context'][-8:]))
