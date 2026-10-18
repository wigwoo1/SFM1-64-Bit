import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert im.string(0x10aa79f0)=='rootControlGroup'
regions=[(0x104d4dd0,0x104d4e66),(0x104d5ba4,0x104d5bbc)]
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
receipt=dict(verified_prefixes=spans,attribute='rootControlGroup',string_va='0x10aa79f0',findings=[
    'Animation set initializer binds wrappercc via104d4dd0(owner,rootControlGroup,nullName,flags0).',
    '104d4dd0 binds type1 via103615e0 and stores attribute pointer at wrapper8; writes CDmeControlGroup metadata10eb61c4 via10363530.',
    'Initial handleffffffff; manager virtual98 true skips creation. False calls manager virtual40 with group type, supplied name, owner38 and final0, using returned handle.',
    'Handle assigned via10366f90; then attribute flags OR((suppliedFlags&ffe0)|800). Callback ordering and conditional creation must be preserved.',
    'Together with104d4a10 this proves animation-set handlecc is rootControlGroup rather than arbitrary node parent relation.'
],limitations='First-line prefixes and name verified against original PE. Manager virtual98/40 semantics, creation failure behavior and original runtime/native graph parity pending.')
Path('outputs/animation-set-root-group-binding-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
