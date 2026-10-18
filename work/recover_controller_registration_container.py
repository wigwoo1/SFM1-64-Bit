import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10431510,0x1043158d)]
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
result=dict(complete_code_bytes=125,routines=groups,findings=[
 '10431510 always guard(9,1,source,263), reads cachedpointer10f4cb30. Cachednonnull bypasses creation. Null witharg0 skipscreation.',
 'Null withnonzeroarg:10432940(source10b5e5c4,line266),10442c60(hash10431320,compare1043cfa0),publish result,10432960,RELOADcache pointer.',
 'Saves selectedpointer before guard(10,1,source,272); returns savedpointer even if guard changes cache.'
],limitations='Container allocation/hash/comparison and guard internals pending; no production service calls executed.')
Path('outputs/controller-registration-container-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified registration container getter:',125,'bytes')
