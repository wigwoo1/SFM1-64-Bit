import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10442b80,0x10442bf8)]
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
result=dict(complete_code_bytes=120,routines=groups,findings=[
 '10442b80 invokes hashcallback8(key), increments raw32 hashcounter38, writes outputhash, bucket=hash%field18; if unsigned bucket<field14 recomputes hash%field10.',
 'Snapshots comparecallback4 before traversal. Each node increments counter58, compares storedhash8; matchinghash increments counter3c and calls snapshotcompare(nodepayload0,key). Zero returns current node slot.',
 'Nonmatchingnode advances via next4, returns address of matching or finalnull link. Bucket divisor zero not guarded. Native node/slot/callback pointers widened; hashes/counters32.'
],limitations='Container sizing/allocation and production registration integration pending; native fixture only.')
Path('outputs/controller-container-search-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified container bucket search:',120,'bytes')
