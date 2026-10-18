import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert im.word(0x10b37094+0xc8)==0x1035adc0
spans=[];publication=[];cursor=0x10357410
active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=0x1035ab1d:break
    if not (0x1035a9d0<=va<0x1035ab1d or 0x10357410<=va<0x103574b1):active=None;continue
    raw=bytes.fromhex(m[2]);assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    span=dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip())
    active=span
    if 0x10357410<=va<0x103574b1:
        assert va==cursor;cursor+=len(raw);publication.append(span)
    else:spans.append(span)
assert cursor==0x103574b1
cursor=0x1035a9d0
for span in spans:
    va=int(span['va'],16);raw=bytes.fromhex(span['bytes']);assert cursor==va
    assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035ab1d
report=dict(intern_verified_complete_instructions=spans,intern_complete_bytes=333,publication_complete_instructions=publication,publication_complete_bytes=161,findings=[
 'Slot50 wrapper1035adc0 calls1035a9d0 at manager68. Null nameFFFFFFFF, existing symbol reused.',
 'Missing name reserves aligned(len+1+7)&~3 bytes via arena103543b0; if absent allocates max(aligned+12,2048) block and registers it. Writes hash+terminated string before10357410.',
 '10357410 first searches primary head toNULL without lock, then read-lock secondary head to primary head stop; read miss acquires write lock and rechecks same range.',
 'Write miss calls10354440 to allocate/link node then writes prepared key pointer at node8; returns canonical node, including duplicate found during retries.',
 'Corrected previous lookup candidate locked argument order: secondary head first, primary head exclusive stop.'
],limitations='Complete333-byte intern and161-byte publication routines verified. Arena allocation, vector registration, concurrent duplicate storage and original execution parity pending; separate node-link/write imports now recovered.')
Path('outputs/owner-symbol-publication-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified complete publication bytes:',161,'intern bytes:',333)
