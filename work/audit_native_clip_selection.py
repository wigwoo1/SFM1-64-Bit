from pathlib import Path
import json,re,struct
from pe_image import Image
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
paths=[root/'bin/tools/ifm.dll',root/'bin/movieobjects.dll',root/'sdktools/python/2.7/win32/Lib/site-packages/vs/datamodel_pyd.pyd']
records=[]
for path in paths:
 if not path.exists():continue
 image=Image(path);matches=[]
 for m in re.finditer(rb'[ -~]{5,}',image.b):
  value=m.group().decode('ascii')
  if not re.search(r'(find|get|active|select).*clip.*(time|track)|(find|get|active|select).*(film|shot)|clip.*at.*time',value,re.I):continue
  if len(value)>500:continue
  va=next((image.base+rva+m.start()-raw for rva,size,raw in image.sections if raw<=m.start()<raw+size),None)
  refs=[]
  if va:
   needle=struct.pack('<I',va);pos=0
   while True:
    pos=image.b.find(needle,pos)
    if pos<0:break
    ref=next((image.base+rva+pos-raw for rva,size,raw in image.sections if raw<=pos<raw+size),None)
    refs.append(hex(ref) if ref else None);pos+=1
  matches.append(dict(text=value,address=hex(va) if va else None,absolute_pointer_candidates=refs))
 records.append(dict(path=str(path),matches=matches))
out=Path('outputs/sfm64-native-clip-selection-locations.json');out.write_text(json.dumps(records,indent=2),encoding='utf-8')
for record in records:
 print(record['path'])
 for m in record['matches']:print(m)
