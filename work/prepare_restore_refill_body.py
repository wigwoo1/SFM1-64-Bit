from pathlib import Path
import hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert hashlib.sha256(im.b).hexdigest()=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
raw=im.b[im.offset(0x10002da0):im.offset(0x10002dec)]
assert len(raw)==76 and raw[39]==0xb9 and raw[44]==0xe8 and raw[53]==0xe8
Path('work/restore_refill_body_init.h').write_text('static const unsigned refillOffset='+str(im.offset(0x10002da0))+';\nstatic const unsigned char refillBytes[]={'+','.join(str(v) for v in raw)+'};\n')
print('restore_refill_original_bytes=76 verified')
