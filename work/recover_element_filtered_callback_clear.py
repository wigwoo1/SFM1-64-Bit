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
    if va>=0x1035e673:break
    if va<0x1035e640:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035e640
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035e673
Path('outputs/element-filtered-callback-clear-recovery.json').write_text(json.dumps(dict(complete_bytes=51,instructions=spans,findings=[
 '1035e640 receives address of list head; captures next8 before freeing each node via10f49a28/1097a990.',
 'Frees ascending list order, including duplicate or nullcallback records; clears head only after entire traversal.',
 'No packed element flag updates, queue effects or callback-object destruction. This is bulk list cleanup, not evidence for individual removal.'
],limitations='Installed bytes verified; allocator lifetime contract, original execution parity, single-subscription removal and native editor integration unresolved.'),indent=2)+'\n')
print('Verified filtered callback clear bytes:',51)
