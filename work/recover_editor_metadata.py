import json,pathlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
interfaces=[]
for i in range(56):
 va=0x10ea0a70+i*8
 interfaces.append(dict(name=im.string(im.word(va)),global_va=hex(im.word(va+4))))
vtable=0x10aeb0f4
locator=im.word(vtable-4)
descriptor=im.word(locator+12)
data=dict(reconnect_table=interfaces,editor_rtti=dict(vtable_va=hex(vtable),locator_va=hex(locator),type_descriptor_va=hex(descriptor),decorated_name=im.string(descriptor+8)))
out=pathlib.Path(__file__).resolve().parent.parent/'outputs'/'sfm-editor-metadata.json'
out.write_text(json.dumps(data,indent=2));print(json.dumps(data,indent=2))
