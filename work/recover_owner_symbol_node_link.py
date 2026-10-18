import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start,end=0x10354440,0x103544ae
cursor=start;spans=[]
active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=end:break
    if va<start:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
for span in spans:
    va=int(span['va'],16);raw=bytes.fromhex(span['bytes']);assert cursor==va
    assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==end
imports={};desc=im.base+im.u32(im.u32(60)+24+104)
while im.word(desc):
    original=im.word(desc);library=im.string(im.base+im.word(desc+12));iat=im.word(desc+16);n=0
    while im.word(im.base+original+n*4):
        value=im.word(im.base+original+n*4)
        if not value&0x80000000:imports[hex(im.base+iat+n*4)]=dict(library=library,name=im.string(im.base+value+2))
        n+=1
    desc+=20
selected={key:imports[key] for key in ['0x10a7b0dc','0x10a7e488','0x10a7e520','0x10a7e51c']}
report=dict(verified_complete_instructions=spans,complete_bytes=end-start,imports=selected,findings=[
 '10354440 sets tabledirtybytec038 before allocator lock acquisition.',
 'Allocator lock at30 supports same-thread recursion; fast owner-thread/CAS0 paths increment depth34, contention invokes imported lock service.',
 'Calls1097ab40 allocator while locked; decrements depth then atomic clears owner when depth0.',
 'After allocator lock release, node4=old bucket4, bucket4=node, node0=prepared key. Does not set node8; publication wrapper setsnode8 after return while bucket write lock held.',
 'New secondary chain is protected by bucket write lock; unlocked readers traverse primary chain, not this new secondary head.'
],limitations='Complete110-byte link routine/imports verified. Allocator1097ab40, primary-chain commit/publish103544b0, lock contention service and original concurrent execution remain unverified.')
Path('outputs/owner-symbol-node-link-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified complete node link bytes:',end-start,'imports:',selected)
