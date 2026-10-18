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
selected={key:imports[key] for key in ['0x10a7d230','0x10a7b30c','0x10a7cd64','0x10a7b4b4','0x10a7b270']}
Path('outputs/editor-ui-import-semantics.json').write_text(json.dumps(selected,indent=2));print(json.dumps(selected,indent=2))
