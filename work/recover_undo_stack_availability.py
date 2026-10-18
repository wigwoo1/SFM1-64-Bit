import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x103906b0,0x103906b9),(0x10390850,0x10390859),(0x10390860,0x10390869)]
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
result=dict(complete_code_bytes=27,routines=groups,findings=[
 '103906b0 reports signed field70>0;10390850 reports undo count0c!=0;10390860 reports signed redo count20>0.',
 'Undo availability is nonzero rather than signed positive; negative count is enabled even though undo loop may do no replay. Preserve this distinction in candidate.'
],limitations='Menu caller bindings and production stack integration pending.')
Path('outputs/undo-stack-availability-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo stack availability:',27,'bytes')
