import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=0x102bdc60:break
    if va<0x102bdbd0:continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
    '102bdbd0 negative grow field8 returns without allocation or capacity update. Required capacity=oldCapacity+additionalCount.',
    'Positive grow field rounds required capacity to next multiple using signed division((required-1)/grow+1)*grow. Zero grow starts at8 if old capacity0, then doubles until sufficient.',
    'Signed overflow fallback averages candidate/required repeatedly if candidate insufficient; overflow edge input invariants remain pending.',
    'Stores capacity4 before allocator call, passes4*capacity bytes. Global allocator10a7e484 virtual0c reallocates nonnull data; virtual4 allocates null data. Stores returned pointer directly, no visible rollback/failure report.',
    'Same helper used by children insertion and reuse ring; original native64 storage must use host-sized pointers while retaining four-byte handle payloads.'
],limitations='First-line prefixes checked against installed PE. Allocator implementation, exceptional arithmetic and failure behavior, negative storage modes and native UI runtime parity unresolved.')
Path('outputs/control-handle-storage-growth-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
