import json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\qtcore4.dll')
expected=bytes.fromhex('558bec8b450c8338007c11837804007c0b8b49048b500c3b5104740d8b4508c700000000005dc208008b4508c700210000005dc20800')
va=0x67112560;at=im.offset(va)
assert im.b[at:at+len(expected)]==expected
p=im.u32(60)+24;d=im.base+im.u32(p+96);names=im.base+im.word(d+32);ords=im.base+im.word(d+36);funcs=im.base+im.word(d+28)
found=False
for i in range(im.word(d+24)):
    name=im.string(im.base+im.word(names+i*4))
    if name.startswith('?flags@QAbstractItemModel@@'):
        ordinal=im.u16(im.offset(ords+i*2));assert im.base+im.word(funcs+ordinal*4)==va;found=True;break
assert found
receipt=dict(export=name,address=hex(va),verified_full_bytes=expected.hex(),findings=[
    'Base flags checks QModelIndex row0 and column4 signed nonnegative. Reads receiver private pointer4 and compares index model0c against private4.',
    'If either coordinate negative or model identity mismatch returns flags0. Otherwise returns21. Complete straight-line routine bytes independently checked, including branch operands.',
    'This verifies base21 assumption for valid owned indices; null node internal pointer does not itself invalidate base index.'
],limitations='Static bundled Qt binary evidence; receiver private layout/model identity construction and actual runtime UI index parity remain unverified.')
Path('outputs/qt-control-model-base-flags-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified complete routine bytes:',len(expected))
