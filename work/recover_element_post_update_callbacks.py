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
    if va>=0x103607f2:break
    if va<0x103603f0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x103603f0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x103607f2
Path('outputs/element-post-update-callback-recovery.json').write_text(json.dumps(dict(complete_bytes=1026,instructions=spans,findings=[
 '103603f0 is callback collection/dispatch, not simple reset. Initializes three temporary vectors with inline16 capacity and external-storage markerFFFFFFFF.',
 'If elementstate3e bit10 set, traverses linked records44/next8; recordmask4 selected when (recordmask & element40 &3)==recordmask, collected through1035fe30.',
 'Normal acyclic element4c list records callback0/value4/next8: collects each callback and corresponding value without deduplication. At nulltail attaches previouselement48, clears48 then swaps4c/48, leaving4cnull and48 concatenated list. Prior48 records not collected in this pass.',
 'First callback vector ascending slot0(element,element40low2), rereads mask before each callback. Then second callback vector ascending slot0(corresponding captured value).',
 'Frees heap-grown temporary vector storage through allocator slot5; external inline storage not freed. No phase reset or elementstate clear inferred here.'
],limitations='Complete bytes verified; list collection/relink semantics/type identities/lifetime and original execution unverified. Do not substitute guessed notification/listener API.'),indent=2)+'\n')
print('Verified post-update callback bytes:',1026)
