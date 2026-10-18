import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start,end=0x10359ca0,0x10359d6c
spans=[];active=None
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
cursor=start
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
selected={key:imports[key] for key in ['0x10a7e528','0x10a7e52c']}
report=dict(verified_complete_instructions=spans,complete_bytes=end-start,imports=selected,findings=[
 'Null source storesFFFFFFFF; nonnull builds stack key with109827f0 hash plus copied terminated string10974fd0.',
 'Bucket index hash&7ff; table starts38, bucket stride24.',
 'First search103543f0(key,bucket0,0) without lock. Miss takes bucket8 CThreadSpinRWLock read lock, searches again from current bucket4 head to bucket0 exclusive stop, then releases read lock.',
 'Still missing storesFFFFFFFF; success stores node8+4 as32-bit symbol token. Symbol represents string storage address in this original table, not ordinal.',
 'Second search examines secondary chain head through primary head boundary; linked-chain helper independently recovered.'
],limitations='Complete204-byte wrapper/imports verified. Hash109827f0, copy helpers, search103543f0 third argument/tree semantics, lock-free first-search concurrency and symbol lifetime remain unverified.')
Path('outputs/owner-symbol-lookup-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified complete symbol lookup bytes:',end-start,'imports:',selected)
