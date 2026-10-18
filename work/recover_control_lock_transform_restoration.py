import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104c6e80,0x104c6ed3),(0x104d27d0,0x104d2836)]
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
'Restoration wrapper104c6e80 passes saved world matrix and48-byte local output to104c6ad0. Resolves receiver6c transform via data-model virtual48; missing transform skips setter, otherwise104d27d0 receives local matrix.',
'Transform setter104d27d0 decomposes input through100802a0 into16-byte,12-byte and4-byte outputs. If existing scale attribute has type3, writes scalar output via1036d040; it does not create missing scale. Writes16-byte output via1036d270 using receiver7c and12-byte output via10372db0 using receiver68. Exact decomposition/helper types still pending.'
],limitations='First-line prefixes checked. World-to-local104c6ad0 and decomposition100802a0 arithmetic still pending; complete transform/scale preservation not proven. No original runtime or native UI parity claim.')
Path('outputs/control-lock-transform-restoration-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
