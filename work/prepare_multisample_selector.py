from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x100297d0,0x10029876
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  pending=None;address=int(m[1],16)
  if start<=address<end:pending={'address':address,'bytes':bytes.fromhex(m[2]).hex()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
covered={}
for row in rows:
 raw=bytes.fromhex(row['bytes']);o=image.offset(row['address']);assert image.b[o:o+len(raw)]==raw
 for i,b in enumerate(raw):covered[row['address']+i]=b
assert set(covered)==set(range(start,end))
raw=image.b[image.offset(start):image.offset(start)+228]
assert raw[18:21]==bytes.fromhex('ff2485') and struct.unpack_from('<I',raw,21)[0]==0x10029878
targets=[image.word(0x10029878+4*i) for i in range(15)]
assert targets==[0x100297e9+9*i for i in range(15)]
for i,target in enumerate(targets):
 o=target-start;assert raw[o]==0xb8 and struct.unpack_from('<I',raw,o+1)[0]==i+2
Path('work/multisample_selector_native_init.h').write_text('#pragma once\nstatic const unsigned char selectorBytes[]={'+','.join(map(str,raw))+'};\nstatic const unsigned selectorOffset='+str(image.offset(start))+';\n')
Path('outputs/sfm64-multisample-selector-preparation.json').write_text(json.dumps({'sha256':hashlib.sha256(image.b).hexdigest(),'verified_code_bytes':166,'table_bytes':60,'table_targets':targets,'policy':'Unsigned request-2 <=14 selects identity2..16, otherwise0.','native_executed':False},indent=2)+'\n')
print('selector_code_verified=166 table_verified=60')
