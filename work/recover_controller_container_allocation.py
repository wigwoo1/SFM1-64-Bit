import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10442c60,0x10442d24)]
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
result=dict(complete_code_bytes=196,routines=groups,findings=[
 '10442c60 allocates96-byte x86 container then64-byte sixteen-pointer bucket array. First allocation failure returnsnull; second failure releasesobject10432900 and returnsnull. Clearsall16slots.',
 'Callbacks comparator4=arg2 or10a182f0; hash8=arg1 or10442c00. Initial fields0c=8,10=16,14=0,18=8,1c=512,20=256; fields24..5c zero.',
 'Native object uses widened pointers/callbacks; buckets require16*sizeof(void*) not original64. Sizing/splitting/growth logic remains to recover.'
],limitations='Native typed layout candidate stores callback identifiers, not executable x86addresses. Allocator wrappers/default callback bodies pending; no production integration.')
Path('outputs/controller-container-allocation-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified container allocator:',196,'bytes')
