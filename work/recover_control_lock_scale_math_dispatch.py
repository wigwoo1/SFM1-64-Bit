import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10a11810,0x10a11864),(0x10a1186d,0x10a118c8)]
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

receipt=dict(verified_prefixes=spans,findings=[
'Math entry10a11810 chooses optimized10a24e70 only if global10ff4a40 nonzero, MXCSR&7f80==1f80 and x87control&7f==7f. Otherwise swaps x87 base/exponent, narrows base to double, stores exponentdouble and calls10a1186d.',
'Fallback positive finite normal branch executesfyl2x (exponent*log2(base)), then10a1f2e0 helper; additional NaN,zero,negative and error paths present. This supports power-operation inference but does not fully identify symbol or prove exceptional-case equivalence.',
'Scale helper compatibility depends on floatingpoint environment and optimized/fallback implementation. A simple std::cbrt substitution would omit actual exponent and this dispatch.'
],limitations='Selected entry and positive branch first-line prefixes checked; fullmathcallee,optimizedtarget,exponentiationhelper and error paths not recovered. Power identity is inference from operation sequence, not named export. Original runtime parity remains pending.')
Path('outputs/control-lock-scale-math-dispatch-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
