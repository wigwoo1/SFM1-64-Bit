import json, re
from pathlib import Path
from pe_image import Image
im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x103623b0,0x10362421),(0x10365f90,0x1036609d),(0x10366f90,0x1036705a)]
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m: continue
    va=int(m[1],16)
    if va>=regions[-1][1]: break
    if not any(a<=va<b for a,b in regions): continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
    '10366000: type must equal1 after masking1f; equal incoming/current handle rejects. If expected type metadata is notffffffff and incoming resolves, virtual10 type test must succeed. Unresolved incoming handle proceeds. Final gate103623b0.',
    '103623b0: flag20 rejects before querying manager10e9ccf8 virtual20. Modes0/1/2 pass; mode4 passes unless flag400; other modes reject. On pass, if flag80 absent and2000 present calls1035e3a0; ORs4080 into flags, calls1035db90, returns true.',
    '10365f90: removes old handle relation via10359940, stores incoming handle through storage pointer, adds new relation via103597e0. Relation helper implementations pending.',
    '10366f90: gate false returns without writes. Gate true optionally allocates24-byte record and calls103659c0, submits through manager virtual13c, then calls10365f90. If flag100 and owner1035dc00 true, invokes owner virtual0(attribute). Calls1035eff0 with element-valued predicate and ORs managerC180 with1000 for element types1/15, otherwise2000.',
    'Record allocation failure still submits null and continues in visible caller; downstream behavior unverified.'
],limitations='First-line prefixes verified against installed PE. Mode enum names, undo record interpretation, relation helpers, owner notifications and runtime parity unresolved. Full native UI integration not yet complete.')
Path('outputs/channel-assignment-path-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
