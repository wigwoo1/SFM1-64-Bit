from pathlib import Path
import re,json,hashlib,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
regions={'generator':(0x100d5020,0x100d5368),'double_math_candidate_span':(0x100e01aa,0x100e1154)}
report={'sha256':hashlib.sha256(im.b).hexdigest(),'regions':{}}
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
for name,(start,end) in regions.items():
 calls={};indirect=[]
 for line in lines:
  address=re.match(r'\s*([0-9A-F]{8}):',line)
  if not address or not start<=int(address[1],16)<end:continue
  direct=re.search(r'\bcall\s+([0-9A-F]{8})\s*$',line)
  if direct:calls.setdefault(direct[1],[]).append(address[1])
  elif re.search(r'\bcall\b',line):indirect.append(line.strip())
 report['regions'][name]={'start':hex(start),'end':hex(end),'direct_calls':calls,'indirect_calls':indirect}
report['limits']='Static call inventory only. Double math span end is the next known float helper, not proven function boundary. Tail jumps, table/data relocations, exception helpers and indirect edges require closure before executing isolated math code. No native table generation or math parity claimed.'
instructions={}
for line in lines:
 m=re.match(r'\s*([0-9A-F]{8}):\s+(?:[0-9A-F]{2}\s+)+(.+)',line)
 if m:instructions[int(m[1],16)]=m[2].strip()
addresses=sorted(instructions)
following=dict(zip(addresses,addresses[1:]))
def reachable(entry):
 pending=[entry];seen=set();calls={};unresolved=[];returns=[]
 while pending and len(seen)<10000:
  address=pending.pop()
  if address in seen:continue
  seen.add(address)
  text=instructions.get(address)
  if text is None:unresolved.append({'address':hex(address),'reason':'missing instruction'});continue
  opcode=text.split()[0]
  target=re.search(r'\b([0-9A-F]{8})\s*$',text)
  if opcode=='ret':returns.append(hex(address));continue
  if opcode=='call':
   calls.setdefault(target[1] if target else 'indirect',[]).append(hex(address))
  if opcode.startswith('j'):
   if target:pending.append(int(target[1],16))
   else:unresolved.append({'address':hex(address),'instruction':text})
   if opcode=='jmp':continue
  if opcode in ('int','int3'):unresolved.append({'address':hex(address),'instruction':text});continue
  if address in following:pending.append(following[address])
 return {'entry':hex(entry),'reachable_instruction_count':len(seen),'lowest_address':hex(min(seen)),'highest_address':hex(max(seen)),'direct_and_indirect_calls':calls,'returns':returns,'unresolved_edges':unresolved,'traversal_limit_hit':bool(pending),'instruction_addresses':[hex(a) for a in sorted(seen)]}
report['reachable_control_flow']={hex(entry):reachable(entry) for entry in (0x100e01aa,0x100e1154)}
report['control_flow_limits']='Calls inventoried but not traversed. Direct tail jumps followed; indirect jumps unresolved. Disassembly decoding and referenced tables/data require byte validation before execution. Reachable address maximum is not a proven contiguous function extent.'
raw_instructions={};current=None
for line in lines:
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
 if m:
  current=int(m[1],16);raw_instructions[current]=bytearray.fromhex(m[2])
 elif current is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):
  raw_instructions[current].extend(bytes.fromhex(line))
 else:current=None
fixups=[]
optional=im.u32(60)+24
rva,length=struct.unpack_from('<II',im.b,optional+96+5*8)
cursor=im.offset(im.base+rva);limit=cursor+length
while cursor<limit:
 page,size=struct.unpack_from('<II',im.b,cursor)
 if not size:break
 assert size>=8 and cursor+size<=limit
 for offset in range(cursor+8,cursor+size,2):
  entry=im.u16(offset)
  if entry>>12==3:fixups.append(im.base+page+(entry&4095))
 cursor+=size
for entry,result in report['reachable_control_flow'].items():
 covered=set()
 for text_address in result['instruction_addresses']:
  address=int(text_address,16);raw=bytes(raw_instructions[address])
  assert raw==im.b[im.offset(address):im.offset(address)+len(raw)],hex(address)
  covered.update(range(address,address+len(raw)))
 relocations=[]
 for address in fixups:
  if address not in covered:continue
  assert all(address+i in covered for i in range(4))
  target=im.word(address)
  try:im.offset(target);backed=True
  except ValueError:backed=False
  relocations.append({'fixup':hex(address),'target':hex(target),'target_file_backed':backed})
 result['verified_instruction_bytes']=len(covered)
 result['absolute_relocations']=relocations
 result['unique_absolute_targets']=sorted({r['target'] for r in relocations})
report['relocation_limits']='HIGHLOW fixups inventoried from PE relocation directory and reachable instruction bytes checked against file. Referenced data extent, indexed-table bounds, mutable state, non-relocated addresses and native runtime parity remain unverified.'
Path('outputs/sfm64-color-generator-dependencies.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({key:{'instructions':value['reachable_instruction_count'],'verified_bytes':value['verified_instruction_bytes'],'relocations':len(value['absolute_relocations']),'unique_targets':value['unique_absolute_targets']} for key,value in report['reachable_control_flow'].items()},indent=2))
