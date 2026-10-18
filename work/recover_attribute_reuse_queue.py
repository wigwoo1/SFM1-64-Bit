import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=0x10354cf7:break
    if va<0x10354c10:continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
    '10354c10 ring enqueue fields: data0,capacity4,grow policy8,head0c,tail10. Empty tailffffffff initializes head/tail0, ensures minimum capacity1 if owned growth permitted, stores supplied value.',
    'Nonempty next tail wraps at capacity. If next equals head, grow via102bdbd0(1), recompute next tail using new capacity. If head nonzero, moves old high segment [head,oldCapacity) upward by capacity delta via10974fe0 then adds delta to head.',
    'Writes new tail then supplied value. Queue relocation preserves FIFO order across wrapped growth. No duplicate-value check.',
    'Pool ring starts at pool18, matching allocator head24/tail28 and release enqueue. Growth implementation and move helper identity remain unresolved.'
],limitations='First-line prefixes verified against installed PE. Complete continuation bytes, allocator failure, external storage policy, growth helper and native integrated runtime parity remain unverified.')
Path('outputs/attribute-reuse-queue-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
