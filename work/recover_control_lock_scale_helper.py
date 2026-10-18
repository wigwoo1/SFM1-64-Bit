import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1007fdf0,0x1007ff04),(0x104986e0,0x10498701)]
spans = []
for line in Path('work/ifm-disassembly.txt').open():
    m = re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)', line)
    if not m:
        continue
    va = int(m[1], 16)
    if va >= regions[-1][1]:
        break
    if not any(a <= va < b for a, b in regions):
        continue
    raw = bytes.fromhex(m[2])
    at = im.offset(va)
    assert im.b[at:at + len(raw)] == raw
    spans.append(dict(va=hex(va), bytes=raw.hex(), instruction=m[3].strip()))

import struct
exponent=struct.unpack_from('<d',im.b,im.offset(0x10aa53a0))[0]
receipt=dict(verified_prefixes=spans,exponent=exponent,findings=[
'Column reader104986e0 reads three matrix components at column offsets0,10,20 into vector via x87 load/store.',
'Scale helper1007fdf0 reads three basis columns and computes each length using SSE squared components/adds/sqrt. Column0 addition orderx2+z2+y2; column1 z2+y2+x2; column2 y2+z2+x2. Multiplies three lengths on x87 stack, loads exponent0.3333333432674408 and calls10a11810. Mathcallee semantics not yet independently identified.',
'Caller stores returned x87 value tofloat before squaring/recriprocal basis correction. This is not a direct single-axis scale or determinant calculation; exponent is widened float approximation rather than exact double1/3.'
],limitations='First-line prefixes and exact double exponent checked. Mathcallee10a11810 and original x87 precision behavior remain unresolved; do not substitute std::cbrt or determinant scale as exact parity.')
Path('outputs/control-lock-scale-helper-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print('exponent:',exponent)
