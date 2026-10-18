import re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for m in re.finditer(rb'[^\x00]{0,70}(?:RenderSettings|SFMSettings|SfmSettings)[^\x00]{0,70}\x00',im.b):print(m.group()[:-1].decode('ascii','replace'))
