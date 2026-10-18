import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10383580,0x1038361d),(0x10383620,0x103836e7)]
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
result=dict(verified_code_bytes=356,routines=groups,default_name=im.string(0x10a8a4f0),findings=['Resolve saved owner handle18 via modelslot18; find saved name14 or fallback10a8a4f0 via1035dd50; require masked attribute type21.', 'Snapshot attribute vector4 before clear10377310. For each old/new snapshot entry, reload live attribute data4 into facade but append index is count0c from the originally captured vector.', 'Undo appends buffer1c/count20; redo appends buffer24/count28. Loop bound and source pointer are live undo-object fields.', 'Redo only calls10377420 afterward when live byte2c nonzero, including empty snapshot.'],limitations='Clear/insert/post-redo helpers and saved-identity base constructor remain delegated.')
Path('outputs/graph-bookmark-undo-methods-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified bookmark undo/redo:',356,'bytes; fallback name:',repr(result['default_name']))
