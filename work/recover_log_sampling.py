import json,struct,pathlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
def refs(va):
 for rva,size,raw in im.sections:
  pos=raw
  while True:
   pos=im.b.find(struct.pack('<I',va),pos,raw+size)
   if pos<0:break
   yield im.base+rva+pos-raw
   pos+=1
records=[]
for name,descriptor in [('float',0x10eb49ac),('vector3',0x10eb49cc),('quaternion',0x10eb4a98),('color',0x10eb4ae4)]:
 for ref in refs(descriptor):
  loc=ref-12
  try:
   if im.word(loc)!=0 or im.word(loc+4)!=0 or im.word(im.word(loc+16))!=0:continue
   for ptr in refs(loc):
    table=ptr+4;entries=[];rva,size,raw=im.sections[0]
    for slot in range(160):
     value=im.word(table+slot*4)
     if not im.base+rva<=value<im.base+rva+size:break
     entries.append(hex(value))
    if len(entries)>60:records.append(dict(type=name,rtti_name=im.string(descriptor+8),locator=hex(loc),vtable=hex(table),sample_slot40=entries[40],predicate_slot60=entries[60],candidate_entries=entries))
  except (ValueError,struct.error):continue
pathlib.Path('outputs/sfm-log-sampling-map.json').write_text(json.dumps(records,indent=2))
for r in records:print(r['type'],r['vtable'],r['sample_slot40'],r['predicate_slot60'])
