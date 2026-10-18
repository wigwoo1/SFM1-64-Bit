from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
ranges=[(0x10045250,0x100452AD),(0x10045420,0x10045445),(0x100502D0,0x10050346),(0x10050530,0x10050555)]
lines=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not any(a<=address<b for a,b in ranges):continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(address);assert image.b[offset:offset+len(prefix)]==prefix;lines.append(line)
for address in (0x10045298,0x100452A1,0x1005032E,0x10050340):assert any(f'{address:08X}:' in line for line in lines)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),verified_instruction_prefixes=len(lines),observations=['10045250 reads its stack argument at ebp+14h as a pointer to two adjacent pointer-sized fields; field0 becomes renderer+A8 and field4 becomes renderer+AC.','A second path around 10050326 binds the same field0/field4 pair from its ebp+14h argument.','Stores around 10045432 and 10050542 clear both fields with ebx; surrounding initialization must establish ebx value before claiming null.'],limitations='Offline pointer binding, not temporal semantics. Caller pair construction, ownership/lifetime, callback/queued behavior and native runtime oracle remain unresolved. Prefix checks exclude continuation bytes.')
Path('outputs/sfm64-native-flex-array-binding.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
Path('outputs/sfm64-native-flex-array-binding-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(f'Native flex arrays supplied as caller pointer pair: {len(lines)} instruction prefixes verified')
