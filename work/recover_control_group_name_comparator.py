import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert im.string(0x10a8a4f0)==''
regions=[(0x103c1480,0x103c14b5),(0x105cb3a0,0x105cb3d5),(0x109751a0,0x1097521e)]
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
    '105cb3a0 dereferences both supplied element-pointer addresses and reads name14; ffffffff replaced with verified empty string10a8a4f0.',
    'Calls109751a0(secondName,firstName); comparison direction reversed relative to supplied arguments. Surrounding sorting call and tie policy pending.',
    'Alternate comparator103c1480 calls109751a0(firstName,secondName), with same name14 and empty fallback. Child sort105cc380 Boolean true selects forward comparison; false selects reverse comparison. Both return full helper value; reverse must not be modeled merely by negating forward because right-terminator raw byte handling can differ.',
    '109751a0 compares unsigned bytes, folds only ASCII uppercase A-Z. Equal strings return0; mismatch returns folded left-minus-right except right terminator returns raw left byte before folding. No locale or natural-number handling.'
],limitations='First-line instruction prefixes checked against PE. Sort caller, UI ordering/ties and original runtime parity unresolved.')
Path('outputs/control-group-name-comparator-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
