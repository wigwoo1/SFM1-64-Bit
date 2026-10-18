from pathlib import Path
import hashlib,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert hashlib.sha256(im.b).hexdigest()=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
raw=im.b[im.offset(0x10002ce0):im.offset(0x10002d2d)]
assert len(raw)==77 and struct.unpack_from('<I',raw,6)[0]==0x100fa0d4
assert all(struct.unpack_from('<I',raw,o)[0]==0x1017e040 for o in [21,55,68])
assert raw[41]==0xe8
Path('work/restore_acquire_init.h').write_text('static const unsigned acquireOffset='+str(im.offset(0x10002ce0))+';\nstatic const unsigned char acquireBytes[]={'+','.join(str(v) for v in raw)+'};\n')
print('restore_acquire_bytes=77 verified')
