import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10498390,0x10498431),(0x10498710,0x10498731),(0x10498a60,0x10498b3c)]
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

mask=[im.word(0x10b84760+4*i) for i in range(4)]
assert mask==[0,0,0,0xffffffff]
receipt=dict(verified_prefixes=spans,translation_mask=mask,findings=[
'10498390 transforms vector by row-major affine matrix: per row ((m1*y + m0*x) + m2*z) + translation; vector input components loaded before output writes.',
'10498710 copies vector into requested matrix column using x87 loads/stores. Snapshot uses column3.',
'10498a60 computes first matrix times second, loading both matrices before any output store. Per output row ((a2*Brow2 + a1*Brow1) + a0*Brow0) + translation-only bitmask of Arow. Original SSE non-fused float arithmetic and operand order recorded.'
],limitations='First-line instruction prefixes and mask checked. x87 exceptional-value behavior not translated; original runtime parity pending. Parent selector identity and lock restoration/animation helpers remain unresolved.')
Path('outputs/control-lock-matrix-helpers-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
