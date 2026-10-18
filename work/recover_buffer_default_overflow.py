import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10022910,0x10022915),(0x1097e2c0,0x1097e30f)]
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
result=dict(complete_code_bytes=84,routines=groups,findings=[
 'Default read overflow10022910 always returns false and has no mutation.',
 'Default write overflow1097e2c0 rejects negative growSize unless flags bit2 set. Allowed external buffer invokes1097dab0(0).',
 'Signed capacity compared to wrapped put-offset+request; repeatedly calls1025f0c0(1) and reloads live fields until capacity meets requirement. No post-conversion ownership check or growth failure exit in this body.'
],limitations='External conversion1097dab0 and grow1025f0c0 remain delegated; allocation failure semantics and production integration pending.')
Path('outputs/buffer-default-overflow-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified default buffer overflow callbacks:',84,'bytes')
