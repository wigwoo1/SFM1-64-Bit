import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x1036d810,0x1036d989),(0x10377870,0x1037789f)]
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
    '10377870 creates adapter10b37dac from wrapper attribute18/storage4, passes wrapper count0c as insertion index and caller handle pointer to1036d810.',
    '1036d810 always resolves incoming handle. If object resolved, virtual10 must accept storage14 type metadata; unresolved handle proceeds. Then103623b0 editability gate; failure returnsffffffff.',
    'Optional manager virtualf8 record path creates24-byte record with requested index and incoming handle; submits virtual13c. Insertion1008f550(storage,index,valuePointer) returns inserted index, used for subsequent callbacks and final return.',
    'If flag100 and owner1035dc00 true, owner virtual4(attribute,index,index) runs after insertion. Incoming relation added via103597e0 using fresh storage[index], then optional owner virtual0(attribute).',
    'Calls1035eff0 and ORs managerC180 mask1000 for element types1/15, otherwise4000. Wrapper retains insertion result; child sort caller ignores it, so rewrite is not proven atomic.',
    'Insertion helper1008f550 and record handle holder1025bf80 remain unresolved; no invented rollback or all-or-nothing semantics.'
],limitations='First-line prefixes checked against installed PE. Full instruction continuation bytes, insertion bounds/growth/failure, owner callback contracts and native runtime UI parity pending.')
Path('outputs/control-children-array-append-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
