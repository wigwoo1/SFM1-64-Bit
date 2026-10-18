import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=0x1008f5b1:break
    if va<0x1008f550:continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
    '1008f550 ensures count+1<=capacity using102bdbd0(count-capacity+1), increments count then stores data pointer in cached10.',
    'If newcount-index-1 positive, invokes10974fe0(destination=data+index+1,source=data+index,bytes=trailing*4).',
    'Reads supplied value pointer only after growth and shift, writes at requested index if destination nonnull; returns requested index unconditionally. No visible index bounds check or allocation failure return.',
    'Child append passes oldcount as index, so trailing shift absent in ordinary child-sort use; generic insertion alias semantics still depend on late source read.'
],limitations='First-line prefixes checked against installed PE. Growth/move helper identities, alias pointer validity across allocation, bounds and runtime native graph parity unresolved.')
Path('outputs/control-handle-array-insert-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
