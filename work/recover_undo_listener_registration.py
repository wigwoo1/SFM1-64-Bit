import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10390f60,0x10390fe3)]
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
result=dict(complete_code_bytes=131,routines=groups,findings=[
 'Listener registration10390f60 scans vector3c/count48 by pointer equality; existing pointer returns false.',
 'Absent pointer appends at original count; grows when count+1 exceeds capacity; refreshes cached base; returns true. Null pointer is not rejected.',
 'Native storage must use pointer-width elements; original scale4 and move byte counts describe x86 storage only.'
],limitations='Growth helper semantics, listener removal/lifetime and production integration pending.')
Path('outputs/undo-listener-registration-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo listener registration:',131,'bytes')
