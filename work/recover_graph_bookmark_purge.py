import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10377420,0x1037754c)]
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
result=dict(verified_code_bytes=300,routines=groups,findings=['This is attribute array purge, not a generic redo completion: clears count and frees owned storage via allocator slot5, zeroes base/capacity and refreshes cached base.', 'Negative grow-size preserves external base/capacity while clearing count. Allocator callback is followed by base reset; cached base reload afterward.', 'Same mutation/recording and owner old-range/changed notifications as clear. Optional undo constructor receives null source,count1,flag0.', 'Redo flag2c calls this purge AFTER restored entries; enabled flag therefore removes restored contents.'],limitations='Flag2c producer/call-site intent and native allocator binding remain unverified. Do not interpret it as shrink-to-fit.')
Path('outputs/graph-bookmark-purge-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified attribute bookmark purge:',300,'bytes')
