from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
ranges=[(0x10057434,0x10057450),(0x100574C0,0x10057555),(0x100502E0,0x10050346)]
lines=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not any(a<=address<b for a,b in ranges):continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(address);assert image.b[offset:offset+len(prefix)]==prefix;lines.append(line)
for address in (0x10057443,0x10057446,0x1005744D,0x10057540,0x10057550):assert any(f'{address:08X}:' in line for line in lines)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),verified_instruction_prefixes=len(lines),observations=['At 10057434 first pointer comes from entry ebp+14h; null is replaced by address10871DA8. It is stored as pair field0 at ebp-10h.','Second pointer comes from entry ebp+18h and is stored as pair field4 at ebp-0Ch; if null, field4 is replaced with the resolved first pointer.','Immediate path passes the pair address at ebp-10h to 100502E0; that routine binds pair fields to renderer+A8/AC or forwards to10045250.','A queued-looking branch passes function100502E0 and local pair to helper10057080 after buffer helper calls; lifetime/copy details are unresolved.'],limitations='Offline pointer fallback and propagation only. Entry symbol/API meaning, incoming array temporal semantics, default backing storage and queue ownership remain unverified. Prefix checks exclude continuation bytes.')
Path('outputs/sfm64-native-flex-pair-construction.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
Path('outputs/sfm64-native-flex-pair-construction-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(f'Native null second weight-array fallback aliases first pointer; {len(lines)} prefixes verified')
