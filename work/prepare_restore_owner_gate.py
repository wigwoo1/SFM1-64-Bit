from pathlib import Path
import hashlib
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert hashlib.sha256(im.b).hexdigest()=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
raw=im.b[im.offset(0x100261a0):im.offset(0x100261c4)]
assert raw.hex()=='83b9a8000000007515f681ac00000002750c83b9cc02000000750333c0c3b801000000c3'
Path('work/restore_owner_gate_init.h').write_text('static constexpr unsigned ownerGateOffset='+str(im.offset(0x100261a0))+';\nstatic const unsigned char ownerGateBytes[]={'+','.join(str(v) for v in raw)+'};\n')
print('owner_gate_original_bytes=36 verified')
