from pathlib import Path
from pe_image import Image
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
def data(name,start,end):
 code=image.b[image.offset(start):image.offset(end)]
 return 'static const unsigned char '+name+'[]={'+','.join(hex(b) for b in code)+'};\n'
Path('work/original_scaler_kernel.inc').write_text(data('originalScaler',0x10a1e21f,0x10a1e580)+data('originalScaleTables',0x10f22800,0x10f22c00))
print('Extracted original inline scaler and table region')
