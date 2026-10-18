
from pathlib import Path
import re,json,hashlib,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  pending={'address':hex(int(m[1],16)),'bytes':bytes.fromhex(m[2]).hex(),'instruction':m[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
selected=[]
for i,row in enumerate(rows):
 text=row['instruction']
 addresses=[int(v,16) for v in re.findall(r'(?<![0-9A-F])([0-9A-F]{8})h',text)]
 if not any(0x1017e8b8<=v<=0x1017f294 for v in addresses) and '000009D8h' not in text:continue
 address=int(row['address'],16);raw=bytes.fromhex(row['bytes'])
 assert im.b[im.offset(address):im.offset(address)+len(raw)]==raw
 selected.append({**row,'context':rows[max(0,i-3):i+4]})
hits={}
for value in [0x1017e8b8,0x1017f188,0x1017f290]:
 needle=struct.pack('<I',value);offset=0;found=[]
 while True:
  offset=im.b.find(needle,offset)
  if offset<0:break
  found.append(offset);offset+=1
 hits[hex(value)]=found
report={'sha256':sha,'verified_reference_instructions':selected,'raw_file_dword_occurrences':hits,
 'finding':'Exact displayed completion-counter references are publisher reads, flush/conditional-refill reads, and static initializer zero store. No advancement writer identified in this inventory.',
 'limits':'Immediate/global operand inventory only, not whole-program alias analysis. Computed pointers, other receiver offsets, external modules, runtime generated code and undisassembled paths may advance completion. Absence here does not prove missing consumer or disabled feature.',
 'next_action':'Trace queue enable initialization and indirect/aliased ring ownership before choosing concurrency implementation.'}
Path('outputs/sfm64-restore-queue-counter-references.json').write_text(json.dumps(report,indent=2)+chr(10))
print('verified_queue_reference_instructions='+str(len(selected))+' raw_exact_counter_occurrences='+str({k:len(v) for k,v in hits.items()}))
