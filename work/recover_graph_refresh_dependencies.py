import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10275f50,0x10275f90),(0x104fd6d0,0x104fd6ee),(0x10741340,0x1074145f)]
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
result=dict(complete_code_bytes=381,routines=groups,findings=['Child update snapshots controller count102bda90; per index reload controller, lookup102c0e70, nonnull virtualslot3.', 'Nested release invokes storedobject slot0(1) ifnonnull, then clears live pointer storage irrespective callback mutation.', 'Element refresh scopes, optional field4e8 path resolvinghandle5c, model recording save/disable/restore around callbacks; clears4e8,1073f430(0), always107358e0.'],limitations='Element refresh semantic recovery partial; scope and model callback identities documented separately.')
result['verified_profile_labels']={hex(v):im.string(v) for v in (0x10bc2784,0x10bc262c,0x10bc24cc)}
result['findings'].append('Embedded profile labels identify10741340 as CDmeGraphEditorCurve::FlattenEditLog and107358e0 as CDmeGraphEditorCurve::UpdateLogBookmarks; graphCurve attribute observed.')
Path('outputs/graph-refresh-dependencies-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified graph refresh dependencies:',381,'bytes')
