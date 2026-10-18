from pathlib import Path
import hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert hashlib.sha256(im.b).hexdigest()=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
raw=im.b[im.offset(0x10020178):im.offset(0x100201fe)]
assert len(raw)==134 and raw[43]==0xe8
Path('work/restore_submission_block_init.h').write_text('static const unsigned blockOffset='+str(im.offset(0x10020178))+';\nstatic const unsigned char blockBytes[]={'+','.join(str(v) for v in raw)+'};\n')
print('restore_submission_block_bytes=134 verified')
