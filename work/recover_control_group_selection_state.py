import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1008c190, 0x1008c210)]
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

receipt=dict(verified_prefixes=spans,entry='0x1008c190',
    findings=[
    'Null node returns0.',
    'Node kind>=4 uses associated control pointer+10. Missing control returns0. Reads selection bits at control+8 and mask table10aa73b8 indexed by signed kind.',
    'Masked bits0 returns1; masked bits equal mask returns2; otherwise returns3. Leaf path returns without writing node+8.',
    'Node kind<4 recurses firstChild+18 and nextSibling+1c; ORs child results, writes aggregate into node+8 and returns it. Empty group stores/returns0.'
    ],limitations='Entire recursive function instruction prefixes checked. Mask-table valid kind domain and user-facing state labels unresolved; codes recorded raw. Graph cycle handling absent in original body; runtime parity pending.')
Path('outputs/control-group-selection-state-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
