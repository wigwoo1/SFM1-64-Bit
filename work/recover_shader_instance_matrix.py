from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert im.word(0x1016f8c8)==0x10057ca7
assert im.word(0x1016f958+38*4)==0x10057cc0
regions=[(0x10057ca7,0x10057cc0),(0x10057cc0,0x10057d31),(0x1005c943,0x1005ca46),(0x100ba198,0x100ba207)]
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
results=[]
for start,end in regions:
 covered={};address=None;instructions=0
 for line in lines:
  m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
  if m:
   address=int(m[1],16);raw=bytes.fromhex(m[2])
   if start<=address<end:instructions+=1
  elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
  else:continue
  for b in raw:
   if start<=address<end:covered[address]=b
   address+=1
 assert len(covered)==end-start
 raw=bytes(covered[a] for a in range(start,end))
 assert raw==im.b[im.offset(start):im.offset(start)+len(raw)]
 results.append(dict(start=hex(start),end=hex(end),bytes=len(raw),instructions=instructions))
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,dispatch_slot=38,default='0x10057cc0',alternate_3dnow='0x100ba198',retained_source_word_indices=[0,4,8,12,1,5,9,13,2,6,10,14],notes=['10057cba is a lazy CPU dispatch thunk, not a PE import.','Initializer copies 74 entries from1016f958 to1016f830; slot38 selects full4x4 transpose.','Default x87 implementation loads/stores selected off-diagonal floats; signaling NaNs can quiet. 3DNow implementation uses integer unpacking and preserves bits.','Instance getter uses64-byte temporary and copies first48bytes into retained matrix storage.'],limitations='Static checked recovery only. Native execution, CPU override selection and floating-point exception parity remain unverified.')
Path('outputs/sfm64-shader-instance-matrix-recovery.json').write_text(json.dumps(report,indent=2))
raw=im.b[im.offset(0x10057cc0):im.offset(0x10057cc0)+113]
Path('work/shader_instance_matrix_native_bytes.h').write_text('#pragma once\ninline constexpr unsigned char shaderInstanceMatrixNativeBytes[]={'+','.join(hex(b) for b in raw)+'};\n')
print('checked_matrix_dispatch_bytes='+str(sum(r['bytes'] for r in results)))
