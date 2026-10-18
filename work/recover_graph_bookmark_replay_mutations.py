import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1008f550,0x1008f5b1),(0x103770a0,0x103771c0),(0x10377310,0x10377419)]
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
result=dict(verified_code_bytes=650,routines=groups,findings=['Clear10377310 uses mutation gate, optional copy-all undo, owner slot2 old inclusive range, clears count, owner slot0 and dirty flags; no owner slot1 notification.', 'Insert103770a0 gates mutation (failure returns -1), optionally records individual insert undo, calls1008f550 then owner slot1(index,index), owner slot0 and dirty flags.', 'Low-level1008f550 grows by wrapped count-capacity+1, increments live count, shifts trailing words and copies pointed rawtime directly without invalid-time initialization.'],limitations='Attribute helper candidates not yet implemented; optional undo/base and owner callbacks remain pending.')
Path('outputs/graph-bookmark-replay-mutations-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified replay mutation helpers:',650,'bytes')
