import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1028bee0,0x1028bf31)]
groups=[[]];active=None
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
names={hex(v):im.string(v) for v in [0x10a8a6c4,0x10ae44a8]}
result=dict(complete_code_bytes=81,routines=groups,names=names,findings=[
 'Editor field80 raw handle resolved by model18 twice; first null exits, second passed to10002830 using first name.',
 'If first named child missing returnsnull, otherwise10002830 again using second name. No cached primary pointer reused.',
 '10002830 attribute lookup1035dd50, typed handle extraction10002780, model18 resolution and element vslot4 token10e9a684 type validation observed; complete helper still pending.'
],limitations='Getter bytes verified; child helper/type identity/second resolve invalidation/runtime integration pending.')
Path('outputs/editor-rate-object-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified rate-object getter:',81,names)
