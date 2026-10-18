import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x102c31d0,0x102c3232),(0x103a3f70,0x103a3f83)]
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
result=dict(complete_code_bytes=117,routines=groups,findings=[
 '103a3f70 removes from nested post-action listener vector3c; shares102c31d0 stable removal with notification vector28.',
 '102c31d0 finds first pointer-equal entry, shifts following entries left with10974fe0 then decrements count; absent returns false. No destructor/free or capacity/cache update.',
 'Duplicates remove only first; null is treated as ordinary pointer. Native move uses pointer-sized elements.'
],limitations='Concrete subscribers and production UI integration pending.')
Path('outputs/undo-post-action-listener-removal-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified post-action listener removal:',117,'bytes')
