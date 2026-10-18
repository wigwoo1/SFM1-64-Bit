import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1008d180, 0x1008d1d2), (0x100907d0, 0x10090af2)]
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

receipt=dict(verified_prefixes=spans, findings=[
'Constructor1008d180 stores kind+0, inherited parent byte+4, zero aggregate+8, invalid handle+c, control record+10, parent+14, null first child+18. Prepends node to parent firstChild via nextSibling+1c.',
'Builder creates common control record with element handle, mask7e for typeglobal10eb160c else80, and zero selection bits.',
'Creates kind4 root. Matching type adds kind9 children12,11,10 then kind5 children8,7,6. Prepending produces child order5,9 and axis-kind orders6,7,8 and10,11,12.',
'All leaf nodes share the control record. Known constructed leaf kinds4..12 map to ffffffff,0e,02,04,08,70,10,20,40.'
], limitations='First-line instruction prefixes verified against installed DLL. This is constructor evidence, not full UI or runtime parity. Semantic kind labels and complete kind domain unresolved. Typeglobal identity not yet independently established.')
Path('outputs/control-tree-leaf-construction-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
