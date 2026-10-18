import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104c2b50,0x104c2b9a)]
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

receipt=dict(verified_prefixes=spans,findings='Ordinaryparent getter104c2b50 interns children relation, calls10223170(receiver,childrenToken,false,mode0). Same fallback path independently recovered in position/rotation selectors. Cycleguard thus follows ordinary incoming children owner and typed overrideParent separately, irrespective of override enable booleans.',limitations='First-line prefixes checked; incoming-reference ordering, runtime parity and native graph adapters unresolved.')
Path('outputs/control-lock-ordinary-parent-getter-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
