import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x104429f0,0x10442ad3)]
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
result=dict(complete_code_bytes=227,routines=groups,findings=[
 'Snapshots split14,modulus18,capacity10; publishes split+1. At unsigned split+1>=oldmod reallocates doublecapacity native-pointer slots. Failure incrementserror5c and resets split0, returns without splitcounter/active increment.',
 'Successful doubling clears slots from LIVEcapacity to newcapacity, sets modulus=LIVEoldcapacity, doublescapacity, split0, publishesnewbase, increments2c. All successpaths incrementactive0c/splitcounter28.',
 'Clears targetbucket oldsplit+oldmod. Traverses oldsplit bucket; nodehash%oldcapacity==oldsplit stays; other nodes unlink then prepend target, reversing moved node order. Uses storedhash, no rehash callback.'
],limitations='Native adapter tests; allocator callback mutation and production integration incomplete.')
Path('outputs/controller-container-resize-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified incremental bucket resize:',227,'bytes')
