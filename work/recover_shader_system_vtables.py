from pathlib import Path
import sys,re,struct,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def va(raw):
 for r,size,off in im.sections:
  if off<=raw<off+size:return im.base+r+raw-off
 raise ValueError('raw')
def refs(address):
 needle=struct.pack('<I',address)
 for r,size,off in im.sections:
  start=off
  while True:
   at=im.b.find(needle,start,off+size)
   if at<0:break
   yield im.base+r+at-off;start=at+1
def executable(address):
 r,size,off=im.sections[0];return im.base+r<=address<im.base+r+size
records=[]
for m in re.finditer(rb'\.\?AV[^\x00]*ShaderSystem[^\x00]*@@\x00',im.b):
 descriptor=va(m.start())-8;name=m.group()[:-1].decode('ascii')
 for ref in refs(descriptor):
  locator=ref-12
  try:
   if im.word(locator)!=0 or im.word(locator+4)>4096:continue
   hierarchy=im.word(locator+16)
   if im.word(hierarchy)!=0:continue
   for ptr in refs(locator):
    table=ptr+4;entries=[]
    for i in range(180):
     entry=im.word(table+i*4)
     if not executable(entry):break
     entries.append(hex(entry))
    if len(entries)<4:continue
    records.append({'name':name,'descriptor':hex(descriptor),'locator':hex(locator),'object_offset':im.word(locator+4),'vtable':hex(table),'slots':entries,'draw_slot18':entries[18] if len(entries)>18 else None})
  except (ValueError,struct.error):continue
report={'materialsystem_sha256':hashlib.sha256(im.b).hexdigest(),'contexts':records,'limitations':'CShaderSystem RTTI tables checked; primary slot18 draw binding recovered. Live service object and GPU execution remain unverified.'}
Path('outputs/sfm64-shader-system-vtables.json').write_text(json.dumps(report,indent=2))
print(json.dumps(records,indent=2))
