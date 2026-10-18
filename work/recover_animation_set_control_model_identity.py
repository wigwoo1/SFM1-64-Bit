from pathlib import Path
import json
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');imports={};pe=im.u32(60);opt=pe+24;desc=im.base+im.u32(opt+104)
while im.word(desc):
 original=im.word(desc);library=im.string(im.base+im.word(desc+12));iat=im.word(desc+16);n=0
 while im.word(im.base+original+n*4):
  value=im.word(im.base+original+n*4)
  if not value&0x80000000:imports[hex(im.base+iat+n*4)]=dict(library=library,name=im.string(im.base+value+2))
  n+=1
 desc+=20
vtable=0x10aa50bc
locator=im.word(vtable-4)
td=im.word(locator+12)
rtti=im.string(td+8)
assert rtti=='.?AVCQAnimationSetControlModel@@'
expected={0x2c:0x1007e840,0x30:0x1007e8d0,0x34:0x1007e930,0x38:0x1007e970,0x40:0x1007f560,0x44:0x1007ef90}
for slot,entry in expected.items():assert im.word(vtable+slot)==entry
selected={key:imports[key] for key in ['0x10a7b94c','0x10a7b958','0x10a7b300']}
receipt=dict(vtable=hex(vtable),locator=hex(locator),type_descriptor=hex(td),rtti=rtti,verified_slots={hex(k):hex(v) for k,v in expected.items()},imports=selected,findings='Recovered navigation routines share CQAnimationSetControlModel vtable. Adjacent1007f560/1007ef90 bodies read model index, column and role; role10 paths use integer QVariant conversion/construction. Full data/setData behavior pending.',limitations='Class RTTI and table entries checked against installed image; method names inferred from navigation and argument patterns, not independently named exports. Qt roles and all column semantics not fully recovered.')
Path('outputs/animation-set-control-model-identity-recovery.json').write_text(json.dumps(receipt,indent=2))
print(json.dumps(receipt,indent=2))
