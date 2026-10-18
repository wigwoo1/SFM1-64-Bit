import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100bf410,0x100bf41b),(0x102cca90,0x102ccae9)]
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
names={hex(v):im.string(v) for v in (0x10a8a6c4,0x10ae9c54)}
result=dict(complete_code_bytes=100,routines=groups,names=names,findings=['Wrapper ignores this, loads global controller then tailcalls102cca90.', 'Snapshot child344; absent=>null. Resolve LIVE childhandle80 via LIVE model slot18; null=>null. Reload handle/model and resolve again; passes secondresult to10002830 with named attribute.', 'Nonnull first attribute result passes102c4420 with second name; result returned. Second resolver result is not independently null-checked.'],limitations='Named-attribute helper implementations delegated; no production session binding.')
Path('outputs/graph-current-selection-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified current-selection lookup:',100,'bytes',names)
