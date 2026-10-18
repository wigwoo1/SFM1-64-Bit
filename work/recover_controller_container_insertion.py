import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10442d30,0x10442dbe)]
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
result=dict(complete_code_bytes=142,routines=groups,findings=[
 '10442d30 ratio=wrapped(itemcount24<<8)/activebuckets0c unsigned, clearerror5c, threshold1c<=ratio calls104429f0 before search.',
 'Search10442b80 returns slot/hash. Existingnode replaces data0 and returnsoldpayload, increments44; doesnot changehash/count or freeoldpayload.',
 'Missingnode allocates12x86bytes; failure incrementsliveerror5c and returnsnull. Success sets hash8,data0,next4null, publishes node at searchslot, increments40 and24, returnsnull. Native pointers/layout widened.'
],limitations='Resize104429f0 and allocator wrapper pending; candidate uses explicit adapter.')
Path('outputs/controller-container-insertion-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified container insertion:',142,'bytes')
