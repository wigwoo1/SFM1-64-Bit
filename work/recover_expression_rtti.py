import json,struct,pathlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
def refs(va):
 needle=struct.pack('<I',va)
 for rva,size,raw in im.sections:
  pos=raw
  while True:
   pos=im.b.find(needle,pos,raw+size)
   if pos<0:break
   yield im.base+rva+pos-raw
   pos+=1
descriptor=0x10eb14f4
assert im.string(descriptor+8)=='.?AVCDmeExpressionOperator@@'
records=[]
for ref in refs(descriptor):
 locator=ref-12
 try:
  if im.word(locator)!=0 or im.word(locator+4)>4096:continue
  hierarchy=im.word(locator+16)
  if im.word(hierarchy)!=0:continue
  for ptr in refs(locator):
   table=ptr+4;entries=[]
   for i in range(48):
    value=im.word(table+i*4)
    rva,size,raw=im.sections[0]
    if not im.base+rva<=value<im.base+rva+size:break
    entries.append(hex(value))
   records.append(dict(locator=hex(locator),object_offset=im.word(locator+4),vtable=hex(table),candidate_entries=entries))
 except (ValueError,struct.error):continue
out=pathlib.Path('outputs/sfm-expression-rtti.json');out.write_text(json.dumps(records,indent=2));print(json.dumps(records,indent=2))
