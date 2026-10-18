import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x104025c0,0x104025d9),(0x104025e0,0x104025fe)]
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
result=dict(complete_code_bytes=55,routines=groups,findings=[
 '104025c0 updates global32state10f4c838 with wrapped state*41c64e6d+3039, stores unrotated state, returns rotateleft16(state).',
 '104025e0 calls10a12d2e(0) to seed shared state then executes generator three times, returning the third result through a tail call.'
],limitations='Seed helper10a12d2e pending; deterministic adapter tests, no original executable oracle or production shared-state integration.')
Path('outputs/controller-startup-generator-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified startup generator/warmup:',55,'bytes')
