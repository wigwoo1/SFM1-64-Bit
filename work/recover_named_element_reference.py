import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10002780,0x100027e0),(0x10002830,0x100028b5)]
groups=[[],[]];active=None
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
result=dict(complete_code_bytes=229,routines=groups,findings=[
 '10002780 initializes static12byte default once: sentinel handle ffffffff at0 and4, zero8. Attribute type flags10 low5==1 returns data pointer4.',
 'Other types call10362e50; true converts via103630a0 into shared scratch10f296fc and returns scratch address; false returns sentinel static.',
 '10002830 separately initializes default static10f29710, looks up name1035dd50, extracts handle through10002780 or default, always resolvesmodel18 including sentinel, then validates resolvedelement vslot4 token10e9a684.',
 'Shared scratch ownership/reentrancy and conversion helpers remain unresolved; do not assume mismatched types simply fail.'
],limitations='Complete bytes verified; conversion helpers/type token initialization/static concurrency/runtime integration pending.')
Path('outputs/named-element-reference-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified typed reference bytes:',229)
