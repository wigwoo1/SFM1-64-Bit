import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104cc040, 0x104cc04f)]
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

assert im.word(0x10b8e958)==0x104cc040
locator=im.word(0x10b8e940)
type_descriptor=im.word(locator+12)
name=im.string(type_descriptor+8)
assert name=='.?AV?$CDmElementFactory@VCDmeTransformControl@@@@'
receipt=dict(verified_prefixes=spans,type_global='0x10eb160c',setter='0x104cc040',factory_vtable='0x10b8e944',setter_slot='0x14',locator=hex(locator),type_descriptor=hex(type_descriptor),factory_rtti=name,findings='Control-tree builder type comparison is CDmeTransformControl: setter stores its runtime type global, and independently verified factory RTTI supplies the type identity.',limitations='Offline setter, vtable, RTTI association verified. Runtime registration and displayed child names remain unresolved. This does not prove complete UI behavior.')
Path('outputs/control-tree-transform-type-recovery.json').write_text(json.dumps(receipt,indent=2))
print(name, 'verified prefixes:',len(spans))
