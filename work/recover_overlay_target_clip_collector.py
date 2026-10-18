import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1008c370, 0x1008c3a3), (0x10128c50, 0x10128f46)]
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

receipt=dict(verified_prefixes=spans,collector='0x10128c50',
    findings=[
    'Clears output vector count. Reads input handles from document+b8 with count+c4.',
    'Both branches resolve handles and require CDmeFilmClip type global10eb1124; unresolved or mismatched elements excluded.',
    'False ordering flag appends eligible elements in document-array order.',
    'True ordering flag inserts eligible elements into20-byte-node tree, using comparator1008c370 and insertion/rebalance helpers10127d20/101287f0.',
    'Comparator reads start through104bba30 and returns first.start < second.start (signed strict less).',
    'Traverses tree using10128520/101285c0, appends node payloads to output, releases tree storage.',
    'Change Overlays passes true, so raw document-array order is not the output order.'
    ],limitations='All collector/comparator instruction prefixes checked, not all continuation bytes. Equal-start traversal/tie behavior and document array selection provenance need proof. Tree helper internals and runtime parity pending.')
Path('outputs/overlay-target-clip-collector-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
