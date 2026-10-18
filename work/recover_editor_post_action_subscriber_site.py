import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100a51f7,0x100a520e)]
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
result=dict(complete_code_bytes=23,routines=groups,findings=[
 'Concrete call site100a51f7 loads known CDataModel global10e9a51c and invokes slot124 with pointer esi+18.',
 'This passes an embedded listener interface, not the complete owning object. Native port must resolve native interface subobject rather than reuse x86 offset18.',
 'Owner class, embedded vtable and callback implementation are not yet identified; nearby unrelated1f0 accesses are not reliable evidence of model registration.'
],limitations='Concrete subscriber class and callback recovery pending; no UI behavior claimed.')
Path('outputs/editor-post-action-subscriber-site-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified concrete embedded subscriber registration:',23,'bytes')
