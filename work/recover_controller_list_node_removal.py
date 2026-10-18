import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10423f80,0x10423fe8)]
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
result=dict(complete_code_bytes=104,routines=groups,findings=[
 '10423f80 nullnode or zerolistcount returns1 no-op. Otherwise unlinks head/middle/tail doubly-linked node (data0,prev4,next8), updates listhead0/tail4.',
 'After unlink invokes listcallback8(contextarg3,nodedata0); releases node through10ea0c34; THEN decrements LIVElistcount0c. Always returns1.',
 'No membership validation, no clearing detached node links and no pre-callback count decrement. Native pointer fields widened; count remains32.'
],limitations='Release callback identity/payload callback implementations pending; native valid-list fixtures, no original executable oracle.')
Path('outputs/controller-list-node-removal-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified list node removal:',104,'bytes')
