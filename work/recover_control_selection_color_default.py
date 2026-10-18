import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1007e720,0x1007e748),(0x10a71790,0x10a717a7)]
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

receipt=dict(verified_prefixes=spans,findings='Color getter1007e720 copies14 bytes from global10fbdaf0 into QColor-like output. Staticinitializer10a71790 invokes QColor::setRgb with r79,g82,b89,a255. These are startup defaults; runtime color writers/preferences remain pending. Partialselection QColor::dark(135), fullselection direct solidbrush established separately.',limitations='First-line prefixes checked. Importsetter signature independently saved in control-selection-color-imports.json. Static initializer execution and runtime overrides not inspected; no screenshot/runtime parity claim.')
Path('outputs/control-selection-color-default-recovery.json').write_text(json.dumps(receipt,indent=2));print('Verified prefixes:',len(spans))
