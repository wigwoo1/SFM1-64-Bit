from pathlib import Path
import sys,re,struct,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
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
for m in re.finditer(rb'\.\?AV[^\x00]*ShaderAPI[^\x00]*@@\x00',im.b):
 descriptor=va(m.start())-8;name=m.group()[:-1].decode('ascii')
 for ref in refs(descriptor):
  locator=ref-12
  try:
   if im.word(locator)!=0 or im.word(locator+4)>4096:continue
   hierarchy=im.word(locator+16)
   if im.word(hierarchy)!=0:continue
   for ptr in refs(locator):
    table=ptr+4;entries=[]
    for i in range(240):
     entry=im.word(table+i*4)
     if not executable(entry):break
     entries.append(hex(entry))
    if len(entries)<4:continue
    records.append({'name':name,'descriptor':hex(descriptor),'locator':hex(locator),'object_offset':im.word(locator+4),'vtable':hex(table),'slots':entries,'capture_layout_slot190':entries[190] if len(entries)>190 else None})
  except (ValueError,struct.error):continue
report={'shaderapidx9_sha256':hashlib.sha256(im.b).hexdigest(),'interfaces':records,'limitations':'RTTI vtable candidates; primary factory returns 10180730, dynamic singleton constructor binding not yet verified. Slot190 from materialsystem capture consumer.'}
Path('outputs/sfm64-shaderapi-layout-vtables.json').write_text(json.dumps(report,indent=2))
print(json.dumps([{'name':r['name'],'vtable':r['vtable'],'offset':r['object_offset'],'slots':len(r['slots']),'layout':r['capture_layout_slot190']} for r in records],indent=2))

# Verify decisive instruction bytes directly against the installed image.
checks={0x1000df20:'b830071810c3',0x100f94a0:'b960041810e896ddf2ff',
0x10027275:'c786d0020000dcce0f10',0x10005bc0:'b8c8f71710c3',
0x100f92a0:'b9c8f71710e8a63ef1ff',0x1000d158:'c706e4bc0f10'}
for address,value in checks.items():
 expected=bytes.fromhex(value);off=im.offset(address)
 assert im.b[off:off+len(expected)]==expected,hex(address)
assert im.word(0x100fcedc+190*4)==0x100262c0
method=im.word(0x100fbce4+29*4)
report['capture_binding']={'interface':'ShaderApi030','registration_initializer':'0x100f94c0',
 'factory':'0x1000df20','interface_object':'0x10180730','complete_object':'0x10180460',
 'interface_offset':720,'constructor':'0x10027240','interface_vtable':'0x100fcedc',
 'layout_slot':190,'layout_forwarder':'0x100262c0','mesh_manager':'0x1017f7c8',
 'mesh_manager_constructor':'0x1000d150','mesh_manager_vtable':'0x100fbce4',
 'mesh_manager_slot':29,'layout_implementation':hex(method),
 'checked_instruction_blocks':len(checks)}
report['limitations']='Selected binding instruction blocks and vtable entries checked against installed binary. Full constructor and forwarder instruction coverage, runtime binding and layout implementation remain unverified.'
Path('outputs/sfm64-shaderapi-layout-vtables.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report['capture_binding'],indent=2))
