from pathlib import Path
import hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert hashlib.sha256(im.b).hexdigest()=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
raw=im.b[im.offset(0x10002d30):im.offset(0x10002d99)]
assert len(raw)==105 and raw[28:34]==bytes.fromhex('8b3dd4a00f10') and raw[-3:]==bytes.fromhex('c20400')
Path('work/restore_publish_init.h').write_text('static const unsigned publishOffset='+str(im.offset(0x10002d30))+';\nstatic const unsigned char publishBytes[]={'+','.join(str(v) for v in raw)+'};\n')
print('restore_publish_bytes=105 verified')
