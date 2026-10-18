import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
table=0x10b37094
assert im.string(im.word(im.word(table-4)+12)+8)=='.?AVCDataModel@@'
lookup=im.word(table+93*4);mark=im.word(table+107*4)
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=0x1035aef4:break
    if va<0x1035ade0:continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
report=dict(verified_prefixes=spans,lookup_slot93=hex(lookup),mark_slot107=hex(mark),findings=[
 '1035ade0 rejects null/empty name with FFFFFFFF. Calls10353980 into104-byte local buffer then virtual lookup slot93.',
 'Existing handle: invokes slot107 and returns existing handle.',
 'Missing handle: poolc19c reserves via1035abe0 before24-byte allocation1025ea80.',
 'Allocated owner initializes symbol0 from interned normalized name, symbol4FFFFFFFF, root8FFFFFFFF, bytec1, count100, field140.',
 'Pool10354b70 binds reserved handle to allocated pointer, including null allocation result; returns reserved handle.',
 'Normalization helper, lookup equality and reservation lifecycle still require recovery; do not invent filesystem canonicalization.'
 ],limitations='Instruction prefixes and CDataModel virtual targets verified. Full continuation bytes, normalization/lookup helpers, slot semantics, allocator/pool failure behavior and session import file binding unverified.')
Path('outputs/element-owner-creation-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified owner creation prefixes:',len(spans),'lookup:',hex(lookup),'mark:',hex(mark))
