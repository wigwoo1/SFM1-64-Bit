import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x104027d0,0x104027ee),(0x1040f020,0x1040f03f)]
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
result=dict(complete_code_bytes=61,routines=groups,findings=[
 '104027d0 returns1 if initialization global10f4c83c already nonzero. Otherwise sets global1 BEFORE tail1040f020; failure does not reset it.',
 '1040f020 calls10430180 then10430020; zero second result returns0 immediately. Nonzero calls10430010,10430000 then returns1, ignoring those helper results.'
],limitations='Inner initialization implementations pending; native adapter tests, no production subsystem initialization performed.')
Path('outputs/controller-startup-guard-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified startup guard/chain:',61,'bytes')
