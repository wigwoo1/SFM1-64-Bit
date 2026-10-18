import json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
pe=im.u32(60);opt=pe+24;desc=im.offset(im.base+im.u32(opt+104))
targets={0x10a7e1c4}
found={}
while im.u32(desc+12):
 dll=im.string(im.base+im.u32(desc+12));lookup=im.u32(desc) or im.u32(desc+16);iat=im.u32(desc+16);i=0
 while True:
  thunk=im.word(im.base+lookup+i*4)
  if not thunk:break
  address=im.base+iat+i*4
  if address in targets:
   name=('ordinal '+str(thunk&65535)) if thunk&0x80000000 else im.string(im.base+thunk+2)
   found[hex(address)]={'dll':dll,'name':name}
  i+=1
 desc+=20
assert len(found)==len(targets)
Path('outputs/controller-subsystem-cleanup-import-bindings.json').write_text(json.dumps(found,indent=2)+'\n')
print(json.dumps(found,indent=2))
