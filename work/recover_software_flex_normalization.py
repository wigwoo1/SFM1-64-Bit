from pathlib import Path
from pe_image import Image
import re,struct,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
ranges=[(0x100010F0,0x10001171),(0x10058A60,0x10058ACD),(0x10013DA0,0x10013DCE)]
lines=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not any(a<=address<b for a,b in ranges):continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(address);assert image.b[offset:offset+len(prefix)]==prefix;lines.append(line)
for address,target in ((0x10013DB4,'100010F0'),(0x1000110A,'10058A60'),(0x1000111B,'10058A60')):
 assert any(f'{address:08X}:' in line and 'call' in line and target in line for line in lines)
epsilon=struct.unpack_from('<f',image.b,image.offset(0x1007076C))[0]
assert epsilon>0
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),verified_instruction_prefixes=len(lines),epsilon=epsilon,epsilon_bits=hex(image.word(0x1007076C)),formula='length=sqrt(float32(float32(x*x+y*y)+z*z)); inverse=1/(length+epsilon); each component multiplied by inverse; all arithmetic float32',observations=['R_StudioFlexVerts calls 100010F0 after its flex loops.','100010F0 iterates output slots at stride 48 and always calls 10058A60 for the first vector, then conditionally for a second vector.','10058A60 computes sum of component squares, sqrtss, reciprocal of length plus epsilon, and writes scaled components.','First normalized vector offset matches normal component stores at output+10h/14h/18h; optional second vector matches output+20h/24h/28h stores.'],limitations='Offline instruction-prefix trace; wrapped continuation bytes excluded. Normal/tangent interpretation follows matching offsets. Exact slot construction, optional flags, native runtime oracle and later skinning remain unverified.')
Path('outputs/sfm64-software-flex-normalization-recovery.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
Path('outputs/sfm64-software-flex-normalization-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(f'Software flex post-loop normalization recovered: {len(lines)} prefixes checked, epsilon={epsilon} bits={report["epsilon_bits"]}')
