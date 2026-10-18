import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1097dab0,0x1097daff)]
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
result=dict(complete_code_bytes=79,routines=groups,findings=[
 '1097dab0 returns without mutation when signed growSize>=0. External negative-growSize snapshots capacity then writes requested growSize before allocation.',
 'Nonzero capacity allocates exactly snapshot capacity, reloads LIVE source pointer after allocation, copies capacity bytes through overlap-aware10a0f1d0, replaces storage. Does not free borrowed storage or change capacity/cursors/flags.',
 'Zero capacity sets storage=null after growSize write. No allocation-null check before copy in original body.'
],limitations='Native candidate assumes successful allocator for nonzero capacity as original copy path does; production allocator failure handling and integration pending.')
Path('outputs/buffer-external-conversion-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified external buffer conversion:',79,'bytes')
