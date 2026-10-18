"""Recover restore boundaries without assigning unproved readiness semantics."""
from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest()
assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
slot=0x100fcedc+183*4
assert im.word(slot)==0x100d7210
assert im.b[im.offset(0x100d7210):im.offset(0x100d7210)+5]==bytes.fromhex('32c0c20400')
regions=[(0x1001fba2,0x1001fbe9),(0x1002114b,0x100211d4)]
rows=[]
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if not m:continue
 address=int(m[1],16)
 if not any(a<=address<b for a,b in regions):continue
 raw=bytes.fromhex(m[2]);offset=im.offset(address)
 assert im.b[offset:offset+len(raw)]==raw
 rows.append({'address':hex(address),'bytes':raw.hex(),'instruction':m[3].strip()})
report={'sha256':sha,'verified_instruction_prefixes':rows,
 'concrete_subobject_table':'0x100fcedc','slot':183,'slot_address':hex(slot),'target':'0x100d7210','target_bytes':'32c0c20400',
 'boundary':'Entry calls subobject183(0), sets owner+30d8=1; tail clears owner+30d8 then calls captured subobject183(1). This concrete slot target returns AL=0 and performs no writes.',
 'tail_snapshot':'Copies 0x39e DWORDs (3704 bytes) owner+1198 to owner+2010, preserving six staging pointers at2858/285c/2860/2864/2868/286c across the copy.',
 'x64_requirement':'A raw 3704-byte native32 state copy cannot be used as an x64 owner layout or pointer ownership transfer. Reconstruct typed snapshot fields and preserve widened owned buffer pointers.',
 'readiness_gating_verified':False,'limits':'Displayed instruction prefixes, concrete slot and five-byte stub verified statically. Other vtable replacements, runtime readiness consumers and full restore execution remain unresolved.'}
Path('outputs/sfm64-restore-guard-boundary.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'verified_boundary_instruction_prefixes={len(rows)} concrete_slot183_stub=1 readiness_gating_verified=0')
