import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10498630,0x10498675),(0x1049d6d0,0x1049d7db)]
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

assert im.word(0x10a8f3c0)==0x80000000
receipt=dict(verified_prefixes=spans,findings=[
'10498630 copies48-byte affine matrix in six ordered8-byte load/store pairs. Full identical alias is supported, arbitrary overlap is not memmove semantics.',
'1049d6d0 transposes3x3 basis. Identical input/output uses swaps, separate pointers copy transpose. Captures inputtranslation before writing outputtranslation. Each output translation is sign-bit xor of ((row1*ty + row0*tx) + row2*tz). No determinant, division, singular check or general inverse.',
'World-to-local routine therefore performs scale adjustment before transpose-based inverse; nonuniform/shear behavior must retain original assumptions rather than substitute a general inverse.'
],limitations='First-line instruction prefixes and sign constant checked. x87 signaling-NaN behavior, scale helper1007fdf0 full return semantics and runtime parity unresolved.')
Path('outputs/control-lock-transpose-inverse-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
