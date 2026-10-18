import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10390bd0,0x10390cdf)]
groups=[[]];active=None
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
result=dict(complete_code_bytes=271,routines=groups,findings=[
 '10390bd0 snapshots signed redo count20; zero returns without context or recording changes.',
 'Collects/begins zero context; saves model slot61 and disables via slot60. Positive snapshot count drives ascending loop; reloads live vector14 each iteration; label slot4/log then record slot5.',
 'Clears live count20 before ending context; restores saved recording state before context storage cleanup. Nonnegative growSize frees owned context base; negative growSize retains external storage.'
],limitations='Complete bytes verified; record slot5 implementation, context helper semantics, allocator binding and native runtime integration remain pending.')
Path('outputs/undo-redo-cleanup-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified redo cleanup:',271,'bytes')
