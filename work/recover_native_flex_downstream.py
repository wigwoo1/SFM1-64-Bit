from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
ranges=[(0x1000F659,0x1000F72D),(0x1000E460,0x1000E72E)]
lines=[];calls=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not any(a<=address<b for a,b in ranges):continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(address);assert image.b[offset:offset+len(prefix)]==prefix
 lines.append(line)
 if re.search(r'\bcall\b',line):calls.append(line.strip())
assert any('1000F697:' in line and '10009BA0' in line for line in lines)
assert any('1000F69F:' in line and '1000E460' in line for line in lines)
assert any('1000F6CF:' in line and '10009DA0' in line for line in lines)
labels={hex(address):image.string(address) for address in (0x100703E8,0x10070400,0x100704A4,0x100704BC)}
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),verified_instruction_prefixes=len(lines),profiling_strings=labels,calls=calls,observations=['Conditional path at 1000F697 calls flex loop 10009BA0, then calls 1000E460, then later calls 10009DA0.','1000E460 has direct helper calls and indirect interface calls that require further tracing.'],limitations='Offline prefix verification; branch activation, helper/interface semantics, downstream normalization and native runtime parity unverified. Wrapped continuation bytes excluded.')
Path('outputs/sfm64-native-flex-downstream-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
Path('outputs/sfm64-native-flex-downstream-recovery.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(labels,indent=2));print(f'{len(lines)} instruction prefixes verified')
