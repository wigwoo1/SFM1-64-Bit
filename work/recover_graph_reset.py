import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100d34e0,0x100d35ae)]
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
result=dict(complete_code_bytes=206,routines=groups,findings=['State168==1 builds padded rectangle from signed min/max endpoints19c/1a0 and1ac/1b0;100d3250(rect178,mode174).', 'States4/5 call100c26b0(owner14+150); others skip specialized work.', 'After callbacks clears168/174/16c, sets byte170=1; invokes import10a7caf0.'],limitations='Rectangle consumer and states4/5 callback semantics pending.')
Path('outputs/graph-reset-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified graph reset:',206,'bytes')
