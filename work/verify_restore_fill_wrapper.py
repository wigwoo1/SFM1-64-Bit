import hashlib,struct,json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
begin=0x100d73a0;raw=im.b[im.offset(begin):im.offset(begin)+9]
assert raw==bytes.fromhex('558bec5de9f77f0000')
target=begin+9+struct.unpack('<i',raw[5:])[0];assert target==0x100df3a0
Path('outputs/sfm64-restore-fill-wrapper-verification.json').write_text(json.dumps({'sha256':sha,'begin':hex(begin),'bytes':raw.hex(),'target':hex(target),'limits':'Only ninebyte wrapper verified; bytefill routine, external fastpath and fullowner utility bindings remain unverified.'},indent=2)+'\n')
print('restore_fill_wrapper_bytes=9 tail_target=0x100df3a0 verified')
