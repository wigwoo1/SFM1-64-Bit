import json, re
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104cbac0, 0x104cbae0), (0x104cc000, 0x104cc040), (0x104cc0a0, 0x104cc137)]
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
    raw = bytes.fromhex(m[2]); at = im.offset(va)
    assert im.b[at:at + len(raw)] == raw
    spans.append(dict(va=hex(va), bytes=raw.hex(), instruction=m[3].strip()))
names = {hex(a): im.string(a) for a in [0x10b8e918, 0x10b8e928]}
assert list(names.values()) == ['defaultPosition', 'defaultOrientation']
receipt = dict(verified_prefixes=spans, attribute_names=names,
    findings=[
        '104cc0a0 calls base constructor10361c40 and installs vtable10b8e96c.',
        'Constructor zeroes fields6c,70,74,78,7c,80,84,8c,98,a4; field88 loads constant10a8a56c.',
        'Constructor initializes fields90,94,9c,a0 to ffffffff. Target resolver104cbf30 previously verified handles90 and9c.',
        '104cbac0 requests defaultPosition with type0a;104cbad0 requests defaultOrientation with type0d.',
        '104cc000 and104cc020 look up these named defaults and use respective global fallback data when missing.'
    ], limitations='First-line instruction prefixes checked against original PE, not every continuation byte. Target handle attribute names and binding routines remain unresolved. No runtime parity or full function recovery claim.')
Path('outputs/transform-control-initialization-recovery.json').write_text(json.dumps(receipt, indent=2))
print('Verified prefixes:', len(spans))
print(names)
