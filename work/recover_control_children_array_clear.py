import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x1036db00,0x1036dbea),(0x103779a0,0x103779c5)]
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
    '103779a0 creates stack adapter with vtable10b37dac,attribute wrapper18 and storage attribute4 then calls1036db00.',
    '1036db00 editability gate103623b0 false returns. True optionally allocates30-byte record via10368710(attribute,0,0,0), submits manager virtual13c; allocation failure submits null and continues in visible caller.',
    'Calls10364f80(adapter,0,count-1) then sets storage count0c to0. No count==0 early return before gate/record/cleanup/notifications.',
    'If flag100 and owner1035dc00 true invokes owner virtual0(attribute). Calls1035eff0 with element-valued predicate types1/15. ManagerC180 OR1000 for element-valued types, otherwise4000.',
    'Original child sort calls clear before append even when filtered child list empty; caller does not inspect clear success.'
],limitations='First-line prefixes checked against installed PE. Cleanup helper, record body, owner notification semantics, append operations and native runtime UI parity pending.')
Path('outputs/control-children-array-clear-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
