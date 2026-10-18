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
    if va>=0x102bdc60:break
    if va<0x102bdbd0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip())
    spans.append(active)
cursor=0x102bdbd0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x102bdc60
report=dict(complete_bytes=144,instructions=spans,findings=[
 '102bdbd0 grows pointer storage at this+0, capacity at+4, signed grow size at+8. Negative grow size returns without allocation.',
 'Positive grow size rounds requested capacity to a multiple; zero grow size starts empty allocation at8 and doubles to meet requested capacity.',
 'Capacity is stored before allocator call. Allocation byte count is native32-bit capacity times4; existing pointer uses allocator virtual slot3, null pointer uses slot1 through imported manager10a7e484.',
 'Intern1035a9d0 passes this=c040 and growth delta=count+1-capacity, then increments countc04c, updates cached pointerc050 and stores new block at old count index.'
],limitations='Complete installed byte verification, not original execution parity. Overflow correction paths present in instructions; no portable growth candidate claimed. Allocator failure contract, external storage behavior, production64-bit pointer sizing and concurrency remain pending.')
Path('outputs/owner-symbol-arena-growth-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified pointer storage growth bytes:',cursor-0x102bdbd0)
