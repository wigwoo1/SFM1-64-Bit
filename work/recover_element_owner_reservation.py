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
    if va>=0x1035ac9b:break
    if va<0x1035abe0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035abe0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035ac9b
report=dict(complete_bytes=187,instructions=spans,findings=[
 'Pool this=c19c: live count0; entry vector4; entry count10; recycled-index ring storage18/capacity1c/head24/tail28.',
 'Nonempty ring reads recycled slot at head. If head==tail sets bothFFFFFFFF; otherwise head advances or wraps at capacity-1. No generation increment in reservation.',
 'Empty ring calls10357810 with entry count10 and vector address4; entry growth/initialization helper remains pending.',
 'Selected entry metadata highbit cleared, pointer4 cleared, live count incremented. Handle is metadata low11 shifted20 plus slotlow20; unrepresentable slot returnsFFFFFFFF after those mutations.',
 'Returned handle highbit comes from ebp-4 initialized by pushECX: incoming pool address highbit. Normal recycled/new paths preserve that address-derived bit. Resolver ignores handle highbit exceptFFFFFFFF sentinel; do not infer a stable semantic flag.',
 'The branch at1035ac3f would load the same local as a slot when selected slotFFFFFFFF, but is unreachable under normal ring invariants.'
],limitations='Complete installed bytes verified; no original execution comparison. Ring invariants/entry growth/free generation policy/import ownership assignment and handlehighbit significance remain pending.')
Path('outputs/element-owner-reservation-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified owner reservation bytes:',cursor-0x1035abe0)
