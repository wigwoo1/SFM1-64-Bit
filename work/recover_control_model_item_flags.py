import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert im.word(0x10aa50bc+0x80)==0x1007ed70
assert im.word(0x10aa50bc+0x48)==0x10719f84
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=0x1007ee83:break
    if va<0x1007ed70:continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
imports={};d=im.base+im.u32(im.u32(60)+24+104)
while im.word(d):
    orig=im.word(d);iat=im.word(d+16);n=0
    while im.word(im.base+orig+n*4):
        v=im.word(im.base+orig+n*4);a=im.base+iat+n*4
        if a in [0x10a7b2fc,0x10a7b298,0x10a7b29c]:imports[hex(a)]=im.string(im.base+v+2)
        n+=1
    d+=20
receipt=dict(verified_prefixes=spans,imports=imports,findings=[
    '1007ed70 first calls imported QAbstractItemModel::flags. Null internal node returns base flags. Column0/other columns return base flags.',
    'Column2 clears bit1; kinds1/2 additionally OR10.',
    'Column3 clears bit1; resolved1008d2d0 result adds10; otherwise kind1 adds50, other kinds add nothing.',
    'Column1: kinds5/9 return(base&~1)|1c. Kind4 resolves control/type guard; transform success returns(base&~1)|5c. All other column1 cases return base&~21.',
    'Model headerData and setHeaderData slots are imported QAbstractItemModel methods; no custom override in this vtable.'
],limitations='First-line prefixes and PE imports checked. Qt flag human-readable names,1008d2d0 complete behavior and runtime view parity pending.')
Path('outputs/control-model-item-flags-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
