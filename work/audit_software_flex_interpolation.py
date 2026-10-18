from pathlib import Path
from pe_image import Image
import re,struct,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
lines=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if match and 0x10013BC3<=int(match[1],16)<0x10013C2C:
  prefix=bytes.fromhex(match[2]);offset=image.offset(int(match[1],16));assert image.b[offset:offset+len(prefix)]==prefix;lines.append(line)
f32=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
unit=struct.unpack_from('<f',image.b,image.offset(0x10070524))[0]
one=struct.unpack_from('<f',image.b,image.offset(0x100701F8))[0]
assert unit==f32(1/255) and one==1
# Symbolic inputs: A=[ebp-8], B=xmm5, C=[ebp-24h], D=[ebp-20h].
# This comparison assumes matching primary/pair semantics, still to be proven.
A,B,C,D=map(f32,(.83,.12,.67,.29))
different=0;max_error=0.;example=None;expected=bytearray()
for speed in range(256):
 for side in range(256):
  s=f32(speed*unit);t=f32(side*unit)
  primary=f32(f32(B*f32(one-s))+f32(A*s))
  paired=f32(f32(D*f32(one-s))+f32(C*s))
  native=f32(f32(primary*f32(one-t))+f32(paired*t))
  expected.extend(struct.pack('<f',native))
  affine_primary=f32(B+f32(s*f32(A-B)))
  affine_paired=f32(D+f32(s*f32(C-D)))
  candidate=f32(affine_primary+f32(t*f32(affine_paired-affine_primary)))
  error=abs(native-candidate);max_error=max(max_error,error)
  if struct.pack('<f',native)!=struct.pack('<f',candidate):
   different+=1
   if example is None:example=dict(speed=speed,side=side,weighted=native,affine=candidate)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),verified_instruction_prefixes=len(lines),symbolic_formula='((B*(1-s)+A*s)*(1-t)) + ((D*(1-s)+C*s)*t), each operation float32',coefficient_inputs=dict(A=A,B=B,C=C,D=D),combinations=65536,different_float32_results=different,max_absolute_difference=max_error,first_difference=example,limitations='Offline operation-order reconstruction; coefficient semantics and record-byte mapping remain provisional. Comparison assumes corresponding primary/pair inputs. No native runtime oracle and no renderer change.')
Path('outputs/sfm64-software-flex-interpolation-audit.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
Path('outputs/sfm64-software-flex-interpolation-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
candidate_path=Path('outputs/software-flex-weight-candidate.bin')
if candidate_path.exists():
 assert candidate_path.read_bytes()==expected,'strict C++ software weight differs from independent float32 operation order'
 report['cpp_strict_candidate_bit_exact']=True
 Path('outputs/sfm64-software-flex-interpolation-audit.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print('Strict C++ candidate matches all 65536 independent float32 results')
print(f'Weighted-sum versus affine interpolation: {different}/65536 float32 differences, max={max_error}')
