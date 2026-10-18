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
    if va>=0x10a62005:break
    if va<0x10a61fb0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x10a61fb0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x10a62005
Path('outputs/callback-inline-detach-recovery.json').write_text(json.dumps(dict(complete_bytes=85,instructions=spans,findings=[
 'If growSize8>=0 returns unchanged. Otherwise sets growSize to argument before allocating.',
 'Nonzero capacity4 allocates capacity*4 bytes via allocator slot1, copies full capacity from borrowed pointer through10a0f1d0, then replaces pointer0. Never frees/reallocates borrowed pointer.',
 'Zero capacity sets pointer0 null without allocator/copy. Capacity unchanged; cache/count not modified.',
 '1035fe30 passes inlinecapacity16 in normal callback temp vector: resulting growth rounds to16-slot multiples rather than doubling.'
],limitations='Complete bytes verified. Allocator/copy failure/overflow/original execution/lifetime pending. Native candidate uses sizeof(pointer) and full capacity copy.'),indent=2)+'\n')
print('Verified inline detach bytes:',85)
