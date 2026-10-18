from pathlib import Path
import struct,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
def va(offset):
 for r,n,p in im.sections:
  if p<=offset<p+n:return im.base+r+offset-p
 raise ValueError(offset)
rows=[]
for name in ['VENGINETOOL003','VENGINETOOLFRAMEWORK003','VCLIENTTOOLS001','VCLIENTENGINETOOLS001']:
 o=im.b.find(name.encode()+b'\0');row={'name':name,'present':o>=0}
 if o>=0:
  address=va(o);row['address']=hex(address);refs=[];start=0
  while True:
   hit=im.b.find(struct.pack('<I',address),start)
   if hit<0:break
   start=hit+1;refs.append(hex(va(hit)))
  row['references']=refs
 rows.append(row)
report={'sha256':hashlib.sha256(im.b).hexdigest(),'interfaces':rows,'disassembly':'work/client-document-gate-disassembly.txt','limitations':'String presence/absence identifies discovery routes only; runtime passed interfaces and dynamically composed names remain possible. Client interfaces must not be assumed to have engine slot92.'}
Path('outputs/sfm64-client-document-tool-interface-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
