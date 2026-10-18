import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x1008cf90,0x1008d019),(0x1008d100,0x1008d173)]
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
 '1008d100 creates kind3, inherited parent byte4 or zero, zero aggregate8, handlec from group10335830 orffffffff initialized through100801c0, null control record10, parent14, null child18, prior parent firstChild as sibling1c, then prepends.',
 '1008cf90 takes element, symbol, mode and restriction. Starts incoming iterator via manager1cc(element handle), saves element owner38, tries1008c3b0 with iterator plus owner, symbol/mode/restriction, returns first nonnull candidate. Advances manager1d0 otherwise.',
 '105cbcf0 caller supplies children symbol, mode1, restriction0. Actual candidate filtering1008c3b0 unresolved; do not replace with arbitrary first incoming reference.'
 ],limitations='First-line prefixes only. Handle wrapper ownership100801c0, iterator candidate1008c3b0 and original runtime parity remain pending.')
Path('outputs/control-group-node-constructor-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified group constructor/incoming iterator prefixes:',len(spans))
