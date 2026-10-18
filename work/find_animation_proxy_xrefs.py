from pathlib import Path
import struct,json,hashlib
from pe_image import Image
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
records=[]
for rel in ['tf/bin/client.dll','bin/tools/ifm.dll','bin/materialsystem.dll','bin/stdshader_dx9.dll']:
 p=root/rel
 if not p.is_file():continue
 im=Image(p);entries=[]
 for term in [b'animatedTextureFrameRate\0',b'animatedTextureVar\0',b'animationNoWrap\0',b'AnimatedTexture\0']:
  start=0
  while True:
   offset=im.b.lower().find(term.lower(),start)
   if offset<0:break
   start=offset+1
   for rva,size,raw in im.sections:
    if raw<=offset<raw+size:
     va=im.base+rva+offset-raw;needle=struct.pack('<I',va);refs=[];search=0
     while True:
      ref=im.b.find(needle,search)
      if ref<0:break
      search=ref+1
      for rr,ss,oo in im.sections:
       if oo<=ref<oo+ss:refs.append(hex(im.base+rr+ref-oo));break
     entries.append(dict(term=term[:-1].decode(),va=hex(va),refs=refs));break
 records.append(dict(binary=str(p),sha256=hashlib.sha256(im.b).hexdigest(),strings=entries))
Path('outputs/sfm64-installed-animation-proxy-string-xrefs.json').write_text(json.dumps(records,indent=2))
print(json.dumps(records,indent=2))
