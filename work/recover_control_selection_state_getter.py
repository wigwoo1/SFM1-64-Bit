import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1008bea0, 0x1008bed2)]
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

receipt=dict(verified_prefixes=spans,entry='0x1008bea0',findings='Nonrecursive read: kinds<4 return cached node+8. Leaf kinds>=4 recompute using control+8 and kind mask; missingcontrol returns0, maskedzero1, complete2, partial3. No writes.',limitations='First-line prefixes verified. Control kind semantic labels/domain unresolved. Earlier nearby dump bytes1008be5c..6f are a switch table, not executable instructions; they are not used as recovered function evidence.')
Path('outputs/control-selection-state-getter-recovery.json').write_text(json.dumps(receipt,indent=2));print('Verified prefixes:',len(spans))
