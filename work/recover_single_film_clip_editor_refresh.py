import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10095b10, 0x10095c89), (0x10095cf0, 0x10095d3d), (0x105c2e60, 0x105c2e67)]
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

receipt=dict(verified_prefixes=spans,controller_getter='0x105c2e60',controller_member='0xb8',selection_helper='0x10095cf0',
    findings=[
    'Controller getter returns memberb8. Selection helper takes clip and force flag; unless forced, compares resolved current clip handle18 against candidate and skips unchanged.',
    'Changed/forced path converts clip pointer to handle (null becomesffffffff), assigns handle18 via1008c280, calls10095b10.',
    'Refresh10095b10 resets/rebuilds member1c linked structures and clears member3c/count48 allocations; uses byte88 to choose one of two element enumeration paths.',
    'Refresh resolves clip handle18, calls104bc1f0 in one branch, then104c2b00/104c2e70 and10090bb0; alternative branch uses10090b00 with type checking.',
    'Refresh resets member5c and calls1008d600(true), with Qt imports around rebuilding pending identification.'
    ],limitations='Full selected helper/refresh instruction prefixes checked, not all continuation bytes. Receiver class, enumeration semantics, Qt imports and final refresh signal unresolved. Do not assume viewport/camera selection; evidence points to rebuilding editor structures. Native UI/runtime parity pending.')
Path('outputs/single-film-clip-editor-refresh-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
