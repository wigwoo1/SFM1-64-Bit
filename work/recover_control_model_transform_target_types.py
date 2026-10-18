import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1027cb60,0x1027cb75),(0x104d2990,0x104d299f),(0x105d2810,0x105d281f)]
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

types=[]
for setter,vt,glob,expected in [(0x104d2990,0x10b8ec7c,0x10eb1914,'.?AV?$CDmElementFactory@VCDmeTransform@@@@'),(0x105d2810,0x10b96378,0x10eb63a4,'.?AV?$CDmElementFactory@VCDmeConstraintSlave@@@@')]:
 assert im.word(vt+0x14)==setter
 col=im.word(vt-4);td=im.word(col+12);name=im.string(td+8);assert name==expected
 types.append(dict(setter=hex(setter),vtable=hex(vt),global_address=hex(glob),locator=hex(col),type_descriptor=hex(td),factory_rtti=name))
receipt=dict(verified_prefixes=spans,types=types,findings='Transformcontrol target lookup accepts CDmeTransform directly, or CDmeConstraintSlave then resolves its handle5c. Intermediate1027cb60 resolves handle80 from receiver pointer after originalminus4 adjustment. Full intermediateobject identity still pending.',limitations='First-line prefixes and factoryRTTI/setters verified. Type registrationruntime IDs, intermediateobject layout/handleattr names and nativegraphadapter parity unresolved.')
Path('outputs/control-model-transform-target-types-recovery.json').write_text(json.dumps(receipt,indent=2));print('Verified prefixes:',len(spans));print(types)
