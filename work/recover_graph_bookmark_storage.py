import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x105079b0,0x105079db),(0x10517210,0x10517258),(0x10517340,0x10517375)]
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
result=dict(complete_code_bytes=168,routines=groups,findings=['Each calls live virtualslot62 then signed component>=count gate; no negative component guard.', 'Count returns field90+component24 orffffffff. Time compares signed index against live storedcount; returns rawword from base84+component24 or0, no negativeindex guard.', 'Setter validcomponent forwards native sourcevector base/count to1038cd00 at targetvector84+component24; invalidcomponent no-op.'],limitations='Underlying vector assignment1038cd00 pending; negative indexes remain original precondition, not sanitized.')
Path('outputs/graph-bookmark-storage-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified bookmark storage wrappers:',168,'bytes')
