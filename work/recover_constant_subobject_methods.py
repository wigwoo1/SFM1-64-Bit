from pathlib import Path
import hashlib,json,re
from pe_image import Image

image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
expected=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text())['sha256']
assert hashlib.sha256(image.b).hexdigest()==expected
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if match:
  pending=dict(address=hex(int(match[1],16)),bytes=bytes.fromhex(match[2]).hex(),instruction=match[3].strip());rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()

def verified_span(start,end):
 selected=[r for r in rows if start<=int(r['address'],16)<end];covered={}
 for row in selected:
  address=int(row['address'],16);raw=bytes.fromhex(row['bytes'])
  assert image.b[image.offset(address):image.offset(address)+len(raw)]==raw
  covered.update({address+i:value for i,value in enumerate(raw)})
 assert set(covered)==set(range(start,end))
 return selected

constructor=verified_span(0x10027240,0x10027289)
assert image.b[image.offset(0x10027275):image.offset(0x10027275)+10]==bytes.fromhex('c786d0020000dcce0f10')
methods=[]
for slot,target in [(41,0x100100a0),(42,0x100101e0),(43,0x100104b0),(44,0x100105f0)]:
 assert image.word(0x100fcedc+slot*4)==target
 assert image.word(0x100fc21c+slot*4)==0x100dee9e
 first_return=next(r for r in rows if int(r['address'],16)>=target and r['instruction'].startswith('ret'))
 end=int(first_return['address'],16)+len(bytes.fromhex(first_return['bytes']))
 selected=verified_span(target,end)
 starts={int(r['address'],16) for r in selected}
 branches=[]
 for row in selected:
  if row['instruction'].startswith('j'):
   operand=row['instruction'].split(None,1)[1]
   if re.fullmatch('[0-9A-F]{8}',operand):
    address=int(operand,16);assert address in starts,(slot,row)
    branches.append(row)
 methods.append(dict(slot=slot,target=hex(target),end_exclusive=hex(end),verified_bytes=end-target,instructions=selected,calls=[r for r in selected if r['instruction'].startswith('call')],branches=branches))
report=dict(dll_sha256=expected,constructor_instructions=constructor,subobject_owner_offset='0x2d0',concrete_vtable='0x100fcedc',transient_base_vtable='0x100fc21c',methods=methods,limits='Verified constructor assignment, table cells and complete method spans through first return; every direct branch stays at decoded instruction start. No native execution or full semantic/ABI binding yet.')
Path('outputs/sfm64-constant-subobject-methods.json').write_text(json.dumps(report,indent=2)+'\n')
print('constant_subobject_methods=4 verified_bytes='+str(sum(m['verified_bytes'] for m in methods)))
