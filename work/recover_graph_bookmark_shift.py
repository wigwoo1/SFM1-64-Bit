import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10974fe0,0x10974fe9),(0x10a161e0,0x10a16200),(0x10a163a0,0x10a163c6)]
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
result=dict(verified_code_bytes=79,routines=groups,findings=['10974fe0 tail-jumps to 10a161e0 with unchanged copy arguments.', 'Unsigned destination greater than source and less than source+count branches to backward-copy path 10a163a0; other cases copy forward.', 'Aligned backward path adjusts to final word, sets direction flag for rep movsd, and clears it afterward.'],limitations='Selected dispatch and backward-copy blocks verified; complete optimized copy implementation, jump tables and SIMD paths not recovered. Native replacement uses standard memmove for valid buffers; no equivalence claim for invalid/wrapped addresses.')
Path('outputs/graph-bookmark-shift-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified bookmark shift dispatch:',79,'bytes')
