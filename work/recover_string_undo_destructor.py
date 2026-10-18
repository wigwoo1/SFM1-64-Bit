import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1025eb70,0x1025eb87),(0x102bcc80,0x102bcca2)]
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
assert slots[8]==0x102bcc80
result=dict(complete_code_bytes=57,routines=groups,vtable=[hex(x) for x in slots],findings=[
 'String undo destructor102bcc80 writes base vtable10a8a9fc unconditionally; frees record through1025eb70 iff argument low bit1 is set; returns original pointer.',
 'No payload owner or old/new interned string release calls occur in this complete destructor.',
 '1025eb70 forwards record pointer to allocator global10a7e484 indirect object virtual slot5.'
],limitations='Allocator object binding and interned string pool lifetime remain external; native runtime integration pending.')
Path('outputs/string-undo-destructor-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified string undo destructor and free dispatch:',57,'bytes')
