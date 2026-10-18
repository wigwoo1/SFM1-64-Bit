import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10353f20,0x10353f6a),(0x103544b0,0x1035455d)]
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=ranges[-1][1]:break
    if not any(a<=va<b for a,b in ranges):active=None;continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
for a,b in ranges:
    cursor=a
    for span in spans:
        va=int(span['va'],16)
        if not a<=va<b:continue
        raw=bytes.fromhex(span['bytes']);assert cursor==va
        assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
        cursor+=len(raw)
    assert cursor==b
report=dict(verified_complete_instructions=spans,complete_bytes=sum(b-a for a,b in ranges),findings=[
 '10353f20 returns immediately ifdirtyc038 zero. Else2048 buckets ascending, read-lock each, primary0=secondary4, unlock; clears dirty flag after all buckets.',
 'This pass does not insert, free or relink nodes; published primary chain becomes initial unlocked lookup range.',
 '103544b0 previously suspected publication is clear/reset: dirty0 before allocator-count gate; write-lock eachbucket, follows chain then clears both heads, unlock; allocator1097a950 reset under allocator lock.',
 'Dirty flag is plainbyte; publication read-locks do not prove safety against concurrent insertion/global dirty races. Caller scheduling contract still required.'
],limitations='Complete74-byte primary publication and173-byte clear routines verified. Allocator reset, caller phase/thread scheduling, lifetime across clear and original concurrent execution parity unverified.')
Path('outputs/owner-symbol-primary-publish-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified complete primary publish/reset bytes:',report['complete_bytes'])
