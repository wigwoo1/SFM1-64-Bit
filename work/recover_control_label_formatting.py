import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x1007ebe0,0x1007ec45),(0x1007ef30,0x1007ef8f),(0x10975830,0x10975873)]
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=regions[-1][1]:break
 if not any(a<=va<b for a,b in regions):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va)
 assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
 '10975830 invokes10a14369(formatter identity still pending), marks truncation for negative result or result>=capacity. Optional out byte receives flag; truncation forces destination[capacity-1] zero and returns capacity.',
 '1007ebe0 append uses stringbuilder length108, destination5+length, capacity255-length; force base byte104 zero, then increments length108 by returned count.',
 '1007ef30 constructs builder vtable10aa51d8, byte4true, destination5 capacity255; forces byte104zero and stores returned length108.',
 'Display capture suffix presence depends on builder length108 (ebp-8), not merely lookup success. Original label buffer holds at most254 formatter characters before terminator at this capacity.'
 ],limitations='First-line prefixes only. CRT10a14369 behavior, builder ownership and exact encoding/formatter parity unresolved. Positive capacity required by original wrapper; no invented zero-capacity safety claim.')
Path('outputs/control-label-formatting-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified label formatting prefixes:',len(spans))
