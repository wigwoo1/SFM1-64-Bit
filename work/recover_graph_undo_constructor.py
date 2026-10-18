import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100bf820,0x100bf892)]
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
result=dict(complete_code_bytes=114,routines=groups,label=im.string(0x10aad9a0),findings=['Publish base vtable10a8aa24, initialize capturedwords8/c toffffffff.', 'Live modelslot80(label) captures firstword into8; reload modelslot81(label) captures firstword into c.', 'Store label pointer4, byte10=0, derivedvtable10aad97c, callerowner14; returnthis.'],limitations='Captured word semantics and undo/redo methods pending; pointer fields widened, capturedwords remain32.')
Path('outputs/graph-undo-constructor-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified graph undo constructor:',114,'bytes; label:',result['label'])
