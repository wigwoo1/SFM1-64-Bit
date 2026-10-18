"""Read MSVC x86 RTTI hierarchy; retain raw PMD and subtree counts."""
import json,pathlib,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
locator=0x10e2dda8;hierarchy=im.word(locator+16)
count=im.word(hierarchy+8);array=im.word(hierarchy+12)
if count>1000:raise ValueError('Implausible hierarchy size')
records=[]
for i in range(count):
 d=im.word(array+i*4);t=im.word(d)
 signed=lambda va:struct.unpack('<i',struct.pack('<I',im.word(va)))[0]
 records.append(dict(index=i,descriptor_va=hex(d),name=im.string(t+8),contained_bases=im.word(d+4),pmd=dict(mdisp=signed(d+8),pdisp=signed(d+12),vdisp=signed(d+16)),attributes=hex(im.word(d+20))))
data=dict(locator_signature=im.word(locator),object_offset=im.word(locator+4),constructor_displacement=im.word(locator+8),hierarchy_va=hex(hierarchy),hierarchy_attributes=hex(im.word(hierarchy+4)),bases=records)
out=pathlib.Path(__file__).resolve().parent.parent/'outputs'/'sfm-editor-inheritance.json'
out.write_text(json.dumps(data,indent=2));print(json.dumps(data,indent=2))
