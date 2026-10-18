import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10335830,0x10335834),(0x10364bd0,0x10364c52)]
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
result=dict(verified_code_bytes=134,routines=groups,findings=['Base undo initializes raw32 captures8/c invalid, independently calls LIVE modelslots80/81 with label, stores label4 and clears byte10.', 'Derived identity table10b37d84; initializes name14 invalid then stores owner element handle1c via10335830(attribute.owner8) into18.', 'Finally reloads attribute interned name14 and stores it into undo.name14. Native names are pointer-sized; handles and capture words remain32bit.', 'Attribute and owner are dereferenced without null guards.'],limitations='Vtable identifiers are tags, not callable x86 addresses. Native model capture binding and lifecycle remain pending.')
Path('outputs/graph-bookmark-undo-identity-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified attribute undo identity:',134,'bytes')
