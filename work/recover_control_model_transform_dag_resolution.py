import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104cbf30,0x104cbff8),(0x104d2940,0x104d2987)]
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

receipt=dict(verified_prefixes=spans,relation=im.string(0x10acfaa8),findings=[
'Transformcontrol helper104cbf30 tries handle90 thenhandle9c via virtual48; resolvedpointer minus4 passed1027cb60. Result type tested against10eb1914; else10eb63a4 and resolves its handle5c. Exact class identities pending.',
'104cbfe0 returnsnull if helper fails, else tailcalls104d2940. That interns relation10acfaa8 and calls incomingreference resolver10223170(result,relation,true,mode0). This requires same owner38 comparison token, unlike normalparent resolverfalse.',
'Consequently control-DAG lookup uses incoming owning relation with same-token restriction; it is not simply reading a DAG pointer directly fromcontrol.'
],limitations='First-line prefixes checked. Intermediate typeglobals/1027cb60 semantics, owner38 meaning and runtime reference ordering unresolved. No complete native graphadapter/UI parity claim.')
Path('outputs/control-model-transform-dag-resolution-recovery.json').write_text(json.dumps(receipt,indent=2));print('Verified prefixes:',len(spans));print('relation:',receipt['relation'])
