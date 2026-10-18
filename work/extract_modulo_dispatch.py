from pathlib import Path
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
def data(name,start,end):
 b=image.b[image.offset(start):image.offset(end)]
 return 'static const unsigned char '+name+'[]={'+','.join(hex(x) for x in b)+'};\n'
Path('work/original_modulo_dispatch.inc').write_text(data('moduloDispatch',0x10a1ebb7,0x10a1ed1a)+data('moduloDescriptor',0x10dfe180,0x10dfe1d0)+data('moduloClassification',0x10e0b320,0x10e0b34c))
