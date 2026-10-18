from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');words={}
for rva,size,raw in image.sections:
 for offset in range(raw,raw+size-3,4):words.setdefault(image.u32(offset),[]).append(image.base+rva+offset-raw)
classes=[]
for match in re.finditer(rb'\.\?AV[^\x00]{1,200}\x00',image.b):
 name=match[0][:-1].decode('ascii',errors='replace')
 if not any(term in name.lower() for term in ('flex','panelmodel','puppet','sfm','studio','modelentity')):continue
 raw=match.start();address=next(image.base+rva+raw-start for rva,size,start in image.sections if start<=raw<start+size);tables=[]
 for field in words.get(address-8,[]):
  locator=field-12
  try:
   if image.word(locator)!=0 or image.word(locator+4)!=4 or image.word(locator+8)!=0:continue
   for pointer in words.get(locator,[]):
    table=pointer+4;tables.append(dict(table=hex(table),slot14=hex(image.word(table+56)),slot39=hex(image.word(table+156))))
  except ValueError:continue
 classes.append(dict(name=name,type_descriptor=hex(address-8),offset4_tables=tables))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),classes=classes,limitations='Name-filtered RTTI scan of one installed client DLL. Offset-4 subobject/table and slot targets are candidates; name similarity is not proof of SFM usage, inheritance or complete override coverage.')
Path('outputs/sfm64-ifm-puppet-weight-candidates.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
for cls in classes:
 if cls['offset4_tables']:print(cls['name'],cls['offset4_tables'])

