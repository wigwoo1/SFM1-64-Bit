import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104990b0,0x104990dd),(0x104c5750,0x104c5863),(0x104d2860,0x104d2939)]
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

assert im.word(0x10a8a56c)==0x3f800000
receipt=dict(verified_prefixes=spans,findings=[
'104990b0 zeros48 bytes then writes1.0 at offsets0,14,28: row-major3x4 affine identity.',
'104c5750 obtains two parent-like pointers104c3ed0/104c3f60. Equal: recursively obtains parent matrix or identity, passes parent and local input to10498a60. Different: obtains each parent matrix separately, saves input translation at offsets0c,1c,2c, calls10498a60 with second parent, transforms saved translation through10498390 with first parent, writes output column3 through10498710. Matrix helper arithmetic still pending.',
'104d2860 constructs matrix from receiver6c/5c via1049dfc0; looks up optional scale attr, requires type3float, scales nine basis elements while preserving translation. Compare branch skips exactly1.0, processes NaN/unordered as well as unequal values.'
],scale_attribute=im.string(0x10aa7864),limitations='First-line instruction prefixes and1.0 constant checked. Parent pointer meaning, quaternion construction and matrix helper multiplication order not fully proven. No complete transform preservation or runtime parity claim.')
Path('outputs/control-lock-snapshot-matrix-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print('scale attribute:',receipt['scale_attribute'])
