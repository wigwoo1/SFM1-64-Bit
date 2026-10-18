import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start,end=0x103543b0,0x103543ed
spans=[];cursor=start
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=end:break
    if va<start:continue
    raw=bytes.fromhex(m[2]);assert va==cursor
    assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw);spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
assert cursor==end
report=dict(verified_complete_instructions=spans,complete_bytes=end-start,findings=[
 '103543b0 captures signed blockcountc04c; count<=0 returnsFFFFFFFF without reading pointertablec040.',
 'Ascending first-fit scan reads block0 capacity minus block4 used with32-bit wrap; signed result compared >= requested argument.',
 'Returns matching ordinal, not byte offset/remaining size; does not reserve or update used bytes. Intern caller updates block used later.',
 'Null block pointer for positive count not checked by original; lifetime/non-nullness caller preconditions.'
],limitations='Complete61-byte first-fit helper verified. Arena allocation/registration/lifetime and concurrent reservation semantics remain unverified; do not claim this scan itself thread-safe or reserves bytes.')
Path('outputs/owner-symbol-arena-fit-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified complete arena first-fit bytes:',end-start)
