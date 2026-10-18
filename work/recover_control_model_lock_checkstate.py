import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1007f7e3,0x1007f8af)]
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
'Column1 role10 branch validates control1008d350, transformcontrol1008c340 and DAG104cbfe0; missing any returns emptyvariant, not unchecked.',
'Reads overridePos/overrideRot through104c3e30. Kind4 returns integer2 ifbothtrue,0 ifbothfalse,1 ifmixed. Kind5 returns2 ifpositiontrueelse0; kind9 returns2 ifrotationtrueelse0. Otherkinds emptyvariant.',
'Checkstate read differs from edit: mixedkind4 toggles both booleans independently on edit; it does not set bothtrue from a partialstate.'
],limitations='First-line prefixes checked. Raw integer states recorded; Qt enum labels understood from surrounding checkbox usage but runtime UI parity pending. Validation helper full control lookup semantics not yet recovered.')
Path('outputs/control-model-lock-checkstate-recovery.json').write_text(json.dumps(receipt,indent=2));print('Verified prefixes:',len(spans))
