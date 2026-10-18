import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104c4300,0x104c4369),(0x104c58e0,0x104c58f8)]
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
'104c4300(receiver,candidate): nullcandidate returns-1; candidate==receiver returns0. Recurses through104c2b50(candidate) and typed overrideParent(candidate). Both recursive calls execute. If bothnegative returns-1; otherwise signedminimum of both results plus1. A missing branch(-1) paired with successful branch can produce0; do not substitute usual shortest-path-depth algorithm.',
'104c58e0 returns recursive result!=-1. Lock writer wrapper103fbfa0 rejects true: prevents candidate that reaches receiver through these parent relations. Existing-cycle handling/visited set absent in recovered routine.'
],limitations='First-line prefixes checked. Normalparent getter104c2b50 body pending; no assumption that recursive return is ordinary distance. No original runtime parity or complete cycle-safe native graph implementation claim.')
Path('outputs/control-lock-parent-cycle-guard-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
