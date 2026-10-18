from pathlib import Path
from pe_image import Image
import json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
targets={0x101D24D0};results=[]
for rva,size,raw in image.sections:
 for offset in range(raw,raw+size-3,4):
  target=image.u32(offset)
  if target not in targets:continue
  entry=image.base+rva+offset-raw
  for slot in range(350):
   table=entry-slot*4
   try:
    locator=image.word(table-4)
    if image.word(locator)!=0 or image.word(locator+8)!=0:continue
    descriptor=image.word(locator+12);name=image.string(descriptor+8)
    if not name.startswith('.?AV'):continue
    # Reject crossing another RTTI table start inside the proposed span.
    crossing=False
    for candidate in results:
     if candidate['entry_value']==entry and candidate['slot']<slot:crossing=True
    if crossing:continue
    results.append(dict(target=hex(target),entry_value=entry,table=hex(table),slot=slot,class_name=name,object_offset=image.word(locator+4)))
   except (ValueError,UnicodeDecodeError):pass
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),matches=results,limitations='Candidate MSVC RTTI association via nearest preceding valid locator within 350 slots. Table extent, actual object use, method names and reset scheduling require further proof.')
Path('outputs/sfm64-model-panel-owner-classes.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
for result in results:print(result['target'],result['class_name'],'slot',result['slot'],'offset',result['object_offset'])

