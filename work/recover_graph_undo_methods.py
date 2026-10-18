import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100bf810,0x100bf818),(0x100bf8a0,0x100bf8b9),(0x100d5370,0x100d5377),(0x100d5380,0x100d5387)]
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
table=[im.word(0x10aad97c+i*4) for i in range(8)]
result=dict(complete_code_bytes=47,routines=groups,vtable=[hex(v) for v in table],findings=['Virtualslot0 loads owner14 and increments ownerfield29c raw32.', 'Virtualslot1 calls live service10e9ccf8 slot10, then reloads owner14 and decrements29c raw32.', 'No counter underflow/saturation/null guards. Undo/redo naming requires stack dispatch proof.'],limitations='Service slot10 meaning pending; no production undo stack integration.')
Path('outputs/graph-undo-methods-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified graph undo methods:',47,'bytes')
