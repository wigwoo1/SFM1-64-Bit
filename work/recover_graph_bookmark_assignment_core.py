import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1038cb80,0x1038ccf8)]
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
result=dict(complete_code_bytes=376,routines=groups,findings=['Same sourcebase as destination: signed size comparison; shrink103883e0(start=count,removed=old-count), grow103771c0(start=old,added=count-old), equal no-op; bypasses outer gate.', 'Distinct source: mutationgate103623b0 false=>return; optional modelslot62(owner) undo creation1036c020 submission79; pre-owner slot2 notification; assignment1038a010; post-owner slot1 then slot0 notifications.', 'Finaltype low5==1/15 controls1035eff0 mark and frameworkc180 OR1000 versus4000; live pointers/count/flags reread around callbacks.'],limitations='Inner growth/copy/undo constructor pending; only alias branch native candidate at present.')
Path('outputs/graph-bookmark-assignment-core-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified bookmark assignment core:',376,'bytes')
