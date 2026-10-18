import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
targets=[im.word(0x10361c24+i*4) for i in range(4)]
assert targets==[0x10361b0e,0x10361b00,0x10361af2,0x10361c0b]
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=0x10361c21:break
    if va<0x10361a40:continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
report=dict(verified_prefixes=spans,jump_targets=[hex(v) for v in targets],findings=[
 '10361a40 optional argument5 lowbyte requires current owner38 equal argument4 original owner before visited lookup.',
 'Visited hash key is element1c; mark before release/store/register prevents cyclic reentry.',
 'Element outgoing attribute list starts30; next attribute word0 read after recursive work.',
 'Restriction0 ignores flags;1 rejects1000;2 requires800 and rejects1000;3 or unsigned>3 skips all outgoing attributes but still changes eligible node owner.',
 'Only type1 scalar and type15 array references traversed; handles resolved through manager slot48 and base CDmElement IsA slot10.',
 'Array count captured before iteration; values accessed through array storage each iteration; ascending index order. Recursive arguments unchanged.',
 '10361eb0 direct phase3 bypasses this graph walker; do not conflate restriction3 here with full setter dispatch.'
],limitations='Instruction first-line prefixes and exact jump-table words checked. Visited hash allocation/insertion103617a0, actual IsA hierarchy, storage mutation lifetime, manager ownership and original execution parity pending. No undo behavior inferred merely from temporary traversal storage.')
Path('outputs/element-owner-graph-walk-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified owner graph prefixes:',len(spans),'restriction targets:',targets)
