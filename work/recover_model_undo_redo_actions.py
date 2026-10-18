import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10353d00,0x10353d2f),(0x10353d30,0x10353d5f)]
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
assert im.word(0x10b37094+126*4)==0x10353b50
result=dict(complete_code_bytes=94,routines=groups,findings=[
 'Model10353d00/30 checks nested c118 undo/redo availability then performs corresponding operation only if available.',
 'After replay reloads live model vtable and tail calls slot126(1f8). Empty availability returns without slot126. Slot126 target10353b50 must be recovered before naming its side effects.'
],limitations='Post-action slot126 semantics and production runtime integration pending.')
Path('outputs/model-undo-redo-actions-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified model undo/redo actions:',94,'bytes')
