import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1035e2b0,0x1035e2e0),(0x1035e2e0,0x1035e32a),(0x1035e330,0x1035e392)]
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
total=sum(end-start for start,end in ranges)
Path('outputs/attribute-callback-remove-recovery.json').write_text(json.dumps(dict(complete_bytes=total,routines=groups,findings=[
 '1035e2b0 membership compares only recordvalue4 to attributepointer; empty/nullheadaddress returnsfalse.',
 '1035e2e0 removes first callback0/value4 pair only, relinks predecessor before returning node to allocator10f49a58 via1097a990. No remove-all duplicate behavior.',
 '1035e330 rejects nullattribute; removes pair from retained48 first, dirty4c only if notfound. If removed, checks attribute membership in both lists and clears flags2000 only when no record remains. Missing pair leaves flag unchanged.'
],limitations='Complete bytes verified. Registration/node allocator/free effects/original execution/UI integration pending; fixtures use explicitfree adapter, not guessed ownership.'),indent=2)+'\n')
print('Verified callback membership/remove bytes:',total)
