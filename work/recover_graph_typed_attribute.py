import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10002830,0x100028b5),(0x102c4420,0x102c44a5)]
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
result=dict(complete_code_bytes=266,routines=groups,findings=['Both wrappers share lazy default state: guard10f2971c bit0, handle10f29710 invalidffffffff, field14 invalidffffffff, field18 zero.', '1035dd50(this,name) then10002780(attribute) or default storage. Dereference32-bit handle and resolve current model slot18.', 'Nonnull element filtered by virtualslot4 using live interned symbol global10e9a684 for settings wrapper or10eff4f4 for graph wrapper; zero low byte=>null.'],limitations='Attribute lookup/accessor and type symbol initialization pending; native symbols pointer width.')
Path('outputs/graph-typed-attribute-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified typed attribute wrappers:',266,'bytes')
