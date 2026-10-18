import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=0x1038f643:break
    if va<0x1038f5e0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1038f5e0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1038f643
Path('outputs/element-callback-queue-recovery.json').write_text(json.dumps(dict(complete_bytes=99,instructions=spans,findings=[
 'Manager vector8c pointer/capacity90/growsize94/count98/cachedpointer9c. Appends rawuint32 elementhandle at captured oldcount.',
 'Ensures count+1 capacity via102bdbd0 delta, increments count, updates cached pointer then writes handle. No duplicate or sentinel filtering.',
 'Normally no shifting because insertionindex=oldcount; generic template shift path only nonzero if growth unexpectedly mutates count. Production growth102bdbd0 does not mutate count.',
 'Original elements remain4-byte handles in x64; widen vector pointer/cached pointer, not handle stride.'
],limitations='Complete byte verification; Consumer1038f160 captures count98, resolves live handles then calls103603f0 and clears98. Runtime binding/lifetime/concurrency/original execution unresolved.'),indent=2)+'\n')
print('Verified element state callback queue bytes:',99)

