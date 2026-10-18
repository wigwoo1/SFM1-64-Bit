from pathlib import Path
from pe_image import Image
import json,re,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
words={}
for rva,size,raw in image.sections:
 for offset in range(raw,raw+size-3,4):words.setdefault(image.u32(offset),[]).append(image.base+rva+offset-raw)
matches=[]
for match in re.finditer(rb'\.\?AV[^\x00]{1,200}\x00',image.b):
 name=match[0][:-1].decode('ascii',errors='replace')
 if name!='.?AVCEngineTool@@':continue
 raw=match.start();descriptor=next(image.base+rva+raw-start-8 for rva,size,start in image.sections if start<=raw<start+size)
 for field in words.get(descriptor,[]):
  locator=field-12
  try:
   if image.word(locator)!=0:continue
   for pointer in words.get(locator,[]):
    table=pointer+4
    matches.append(dict(name=name,table=hex(table),offset=image.word(locator+4),slots=[hex(image.word(table+slot*4)) for slot in range(55)]))
  except ValueError:pass
for item in matches:item['table_pointers']=[hex(v) for v in words.get(int(item['table'],16),[])]
interfaces=[]
for match in re.finditer(rb'VENGINETOOL003\x00',image.b):
 raw=match.start();address=next(image.base+rva+raw-start for rva,size,start in image.sections if start<=raw<start+size)
 interfaces.append(dict(address=hex(address),aligned_pointers=[hex(v) for v in words.get(address,[])]))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),matches=matches,interfaces=interfaces,limitations='RTTI candidate tables; registration and method semantics require independent verification.')
Path('outputs/sfm64-engine-tool-tables.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
