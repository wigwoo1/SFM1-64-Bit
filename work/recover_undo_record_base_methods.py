import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10009310,0x1000931e),(0x10009360,0x10009364),(0x10009370,0x1000937d),(0x10676490,0x10676494)]
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
vt=0x10b3820c
slots=[im.word(vt+i*4) for i in range(9)]
assert slots[4:8]==[0x10676490,0x10009310,0x10009360,0x10009370]
result=dict(complete_code_bytes=35,routines=groups,vtable=[hex(x) for x in slots],findings=[
 'String undo slot5 at10009310 null guards this then invokes virtual slot8 with argument1. Actual destructor target captured separately, not recovered here.',
 'Slot4 returns label pointer field4. Slot6 reads raw marker byte10. Slot7 stores low input byte directly, not normalized boolean.'
],limitations='Destructor slot8, retained-data ownership and production runtime integration remain pending.')
Path('outputs/undo-record-base-methods-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo record base methods:',35,'bytes; destructor',hex(slots[8]))
