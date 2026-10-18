import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x102bdbd0,0x102bdc60)]
groups=[[] for _ in ranges];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16);active=None
    if va>=ranges[-1][1]:break
    for i,(start,end) in enumerate(ranges):
        if start<=va<end:
            active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());groups[i].append(active);break
for (start,end),spans in zip(ranges,groups):
    cursor=start
    for s in spans:
        va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
        assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
        cursor+=len(raw)
    assert cursor==end
result=dict(complete_code_bytes=144,routines=groups,findings=['Negative grow-size returns unchanged. Positive grow-size rounds requested capacity to a grow-size multiple; zero grow-size starts at eight elements and doubles.', 'Capacity is published before allocation. Existing storage uses allocator virtual slot3; absent storage uses slot1. Allocation byte count is raw32 capacity*4.', 'Allocation result replaces storage even when null; no failure rollback. Overflow fallback retains signed wrapped arithmetic.'],limitations='Allocator binding and pathological nonterminating overflow inputs remain unverified.')
Path('outputs/graph-bookmark-growth-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified bookmark growth:',144,'bytes')
