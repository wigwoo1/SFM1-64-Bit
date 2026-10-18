import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=0x10364fe4:break
    if va<0x10364f80:continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
    '10364f80 reads attribute owner8; if attribute flag100 and owner1035dc00 true invokes owner virtual8(attribute,first,last) before checking range emptiness.',
    'Then loops signed inclusive first..last; each iteration reloads adapter attribute4 and storage8/data0, reads handle[index], calls10359940(handle,attribute). Does not copy handle range before callback.',
    'Clear caller passes first0,last=count-1; empty array still permits owner callback0/-1 but no reference removal iterations.',
    'No bounds/type checks in cleanup body. Caller range and callback mutation invariants remain unresolved.'
],limitations='First-line prefixes verified against original PE. Owner callback meaning and mutation contract, signed overflow invariants, runtime graph parity and UI integration pending.')
Path('outputs/children-reference-cleanup-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
