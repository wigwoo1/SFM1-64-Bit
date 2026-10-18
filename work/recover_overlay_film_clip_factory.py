import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104c1a60, 0x104c1a6f)]
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

assert im.word(0x10b8e264) == 0x104c1a60
locator = im.word(0x10b8e24c)
type_descriptor = im.word(locator+12)
name = im.string(type_descriptor+8)
assert name == '.?AV?$CDmElementFactory@VCDmeFilmClip@@@@'
receipt = dict(verified_prefixes=spans, type_global='0x10eb1124', setter='0x104c1a60',
    factory_vtable='0x10b8e250', setter_slot=hex(0x10b8e264-0x10b8e250),
    locator=hex(locator), type_descriptor=hex(type_descriptor), factory_rtti=name,
    findings='Overlay creation helper10127b50 reads type global10eb1124. Its setter104c1a60 is in CDmElementFactory<CDmeFilmClip> vtable at slot0x14. Factory type identity independently established as film clip.',
    limitations='RTTI/vtable/setter association checked against installed image. Registration invocation and assigned runtime type ID remain unresolved; no complete constructor or native creation parity claim.')
Path('outputs/overlay-film-clip-factory-recovery.json').write_text(json.dumps(receipt, indent=2))
print(json.dumps(dict(factory_rtti=name,verified_prefix_count=len(spans)),indent=2))
