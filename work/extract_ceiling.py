from pathlib import Path
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
def data(name,start,end):
 b=image.b[image.offset(start):image.offset(end)]
 return 'static const unsigned char '+name+'[]={'+','.join(hex(x) for x in b)+'};\n'
Path('work/original_ceiling.inc').write_text(data('originalCeiling',0x10a0f010,0x10a0f0ed)+data('ceilingConstants',0x10dfe100,0x10dfe150))
print('Extracted optimized ceiling routine and constants')
