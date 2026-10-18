import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x10353fe0,0x10354072),(0x10357810,0x10357879)]
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=regions[-1][1]:break
    if not any(a<=va<b for a,b in regions):continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
    '10357810 inserts one8-byte slot at requested index; ensures count+1 capacity via10353fe0, then increments count and updates cached data pointer. Moves following slots through10974fe0 when insertion not at end; helper identity pending.',
    'New slot writes both words zero; normal caller1035ab20 passes previous count, so no following-slot move and initial generation/state word is zero.',
    '10353fe0 returns without growth when grow-size field8 negative. Positive grow size rounds required capacity upward to multiple. Zero grow size begins at4 if current capacity zero, otherwise doubles until sufficient.',
    'Fallback signed arithmetic path averages candidate with required capacity when candidate still too small; overflow edge semantics require further verification.',
    'Capacity field4 is stored before allocation. Uses global allocator10a7e484: nonnull old pointer calls virtual0c(old,sizeBytes); null calls virtual4(sizeBytes). Stores returned pointer directly, no visible rollback/null guard.'
],limitations='First-line instruction prefixes checked against installed PE. Allocation interfaces, move helper identity, exceptional arithmetic, allocation failure and token reuse generations remain unverified. No complete allocator or runtime parity claim.')
Path('outputs/attribute-slot-growth-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
