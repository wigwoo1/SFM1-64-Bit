import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x10a1f470,0x10a1f4b3),(0x10a1f5d1,0x10a1f5d4)]
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
 '10a1f470 always first calls10a1859b(stream); descriptor helper effects remain pending.',
 'After descriptor lookup, stream flags0c lacking82 set helper10a16f8b pointed value9, ORflags20 and return-1.',
 'Flags including40 set helper10a16f8b pointed value22h, ORflags20 and return-1 through same epilogue.',
 'Label formatter stream initializedflags42 therefore follows40 rejection when terminator remaining-capacity decrement went negative. No later file flush/write branches execute in this case.',
 'Outer10975830 sets truncation for negative returned formatter count or count>=capacity, overwrites last byte with zero and returns capacity. Terminator rejection return ignored by10a142bd; original formatter count retained.'
 ],limitations='First-line prefixes only. Error helper identity/descriptors and formatter count/parser semantics still pending; no full file-stream translation or original runtime parity claim.')
Path('outputs/label-stream-overflow-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified label stream overflow prefixes:',len(spans))
