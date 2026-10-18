import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=0x10357879:break
    if va<0x10357810:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x10357810
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x10357879
Path('outputs/element-owner-entry-insert-recovery.json').write_text(json.dumps(dict(complete_bytes=105,instructions=spans,findings=[
 'Vector this=pool+4: pointer0/capacity4/countc/cached pointer10. Ensures count+1 via10353fe0 before incrementing count.',
 'After count increment updates cached pointer. Interior insertion shifts trailing entries via10974fe0 memmove using8-byte native stride.',
 'New entry metadata0 and object pointer4 initialized tozero; returns requested insertion index. Reservation passes oldcount, so no shift and new generation starts0.'
],limitations='Original execution/growth helper/allocator failure/invalid insertion preconditions unverified. x64 entries must account for widened object pointers; original8-byte stride cannot be reused.'),indent=2)+'\n')
print('Verified owner entry insert bytes:',105)
