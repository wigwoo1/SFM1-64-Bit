from pathlib import Path
from pe_image import Image
import re,struct,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
ranges=[(0x100010F0,0x10001171),(0x10013C7D,0x10013C9C)]
lines=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not any(a<=address<b for a,b in ranges):continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(address);assert image.b[offset:offset+len(prefix)]==prefix;lines.append(line)
constants={hex(address):struct.unpack_from('<f',image.b,image.offset(address))[0] for address in (0x100701F8,0x100701F4,0x100701F0)}
assert constants=={'0x100701f8':1.,'0x100701f4':-1.,'0x100701f0':2.}
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),verified_instruction_prefixes=len(lines),constants=constants,offsets=dict(output_slot='owner+4+48*slot',scalar='output+0x0c',normal='output+0x10',tangent='output+0x20',tangent_fourth='output+0x2c'),observations=['Final helper edi=owner+0x10 for first slot, equivalent to output+0x0c; esi=edi+0x14, equivalent to output+0x20.','Conditional scalar finalization reads output+0x0c, bounds finite values to [-1,+1], adds 2, multiplies by output+0x2c, and writes output+0x0c.','Tangent fourth component is read but not written by this block.','Accumulator writes wrinkle-record extra signed-short contribution to output+0x0c.'],limitations='Offline layout/branch interpretation. Scalar shader meaning, optional flag selection and exceptional float behavior remain unverified. Original runtime not executed and continuation bytes excluded.')
Path('outputs/sfm64-software-flex-scalar-finalize.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
Path('outputs/sfm64-software-flex-scalar-finalize-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print('Scalar finalization writes output+0x0c, reads tangent fourth; constants -1, 1, 2 verified')
