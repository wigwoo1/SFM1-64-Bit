import json,pathlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
locator=im.word(0x10ae7154);descriptor=im.word(locator+12)
data=dict(class_name=im.string(descriptor+8),vtable='0x10ae7158',locator=hex(locator),restore_literals={hex(a):im.string(a) for a in [0x10aa9770,0x10aa7424,0x10ac3ab8,0x10ae6600,0x10ae65dc]})
out=pathlib.Path(__file__).resolve().parent.parent/'outputs'/'sfm-document-identity.json'
out.write_text(json.dumps(data,indent=2));print(json.dumps(data,indent=2))
