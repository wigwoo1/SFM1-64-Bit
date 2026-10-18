import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10a0f1d0,0x10a0f1f0),(0x10a0f390,0x10a0f3b6)]
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
result=dict(verified_branch_bytes=70,routines=groups,findings=[
 '10a0f1d0 loads destination,source,count; unsigned destination<=source takes forward path. Destination inside (source,source+count) branches10a0f390.',
 '10a0f390 establishes tail pointers and backward copying; aligned count>=32 uses STD REP MOVSD CLD. Overlap-aware move semantics require memmove rather than memcpy in extraction candidate.',
 'This verifies overlap dispatch and one backward path only; complete optimized implementation including10a1e5b3 not recovered by this receipt.'
],limitations='Semantic inference for valid native storage; not a complete function or native original oracle.')
Path('outputs/buffer-copy-overlap-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified overlap dispatch/backward path:',70,'bytes')
