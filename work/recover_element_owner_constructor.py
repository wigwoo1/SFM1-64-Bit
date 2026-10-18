"""Trace verified owner-field initialization; do not assign a semantic file ID yet."""
import json, re
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert im.word(0x10b3737c) == 0x103621b0
locator = im.word(0x10b3737c - 4)
assert im.string(im.word(locator + 12) + 8) == '.?AV?$CDmElementFactory@VCDmElement@@@@'
spans = []
for line in Path('work/ifm-disassembly.txt').open():
    m = re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)', line)
    if not m:
        continue
    va = int(m[1], 16)
    if va >= 0x103621ed:
        break
    if not (0x10361c40 <= va < 0x10361d26 or 0x103621b0 <= va < 0x103621ed):
        continue
    raw = bytes.fromhex(m[2])
    at = im.offset(va)
    assert im.b[at:at + len(raw)] == raw
    spans.append(dict(va=hex(va), bytes=raw.hex(), instruction=m[3].strip()))
assert im.b[im.offset(0x10361c9b):im.offset(0x10361ca1)] == bytes.fromhex('8b4518894638')
report = dict(verified_instruction_prefixes=spans, findings=[
    'CDmElement factory vtable slot0 is103621b0; allocates5c bytes and invokes10361c40.',
    'Factory argument3 (ebp+10) becomes constructor argument5 (ebp+18). Factory argument5 becomes constructor argument3.',
    'Constructor10361c40 stores its fifth argument directly into element+38 at10361c9b/10361c9e.',
    'Constructor then passes element+1c and element+38 into helper10356870 via manager10e9a518.',
    'Owner38 is not derived from element UUID or element handle by this constructor.'
], limitations='Factory RTTI and instruction prefixes checked against installed PE. Complete continuation bytes, allocator behavior, registration helper10356870, caller argument semantics and original runtime parity remain unverified. Do not label owner38 a file ID yet.')
Path('outputs/element-owner-constructor-recovery.json').write_text(json.dumps(report, indent=2) + '\n')
print('Verified constructor/factory prefixes:', len(spans))
