import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1007e7b0,0x1007e839),(0x1007e840,0x1007e8c9),(0x1007e8d0,0x1007e922),(0x1007e930,0x1007e965),(0x1007e970,0x1007e978),(0x1008c6c0,0x1008c6d9),(0x1008c6e0,0x1008c748)]
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
'1008c6c0 counts firstChild18/nextSibling1c. 1008c6e0 searches this sibling chain by pointer, returns zero-based ordinal or -1.',
'1007e840 uses valid parent index node or resolves model root via member8 and1035dc10; asks1008be70 for requested child ordinal. Missing root/child yields invalid index. Success stores requested row/column, child pointer and model owner.',
'1007e8d0 rejects invalid input, missing node or missing parent; constructs parent index through1007e7b0. That helper rejects model root, parentless nodes, or nodes missing from their parent sibling chain.',
'1007e930 resolves valid index node or fallback root then returns child count; missing root returns0. 1007e970 returns4 columns unconditionally.'
],limitations='First-line prefixes checked against installed DLL. QModelIndex-like record identification from call pattern; Qt API/vtable roles and full model data implementation need independent confirmation. Child accessor1008be70 begins at a dump misalignment boundary and is not included as fully verified instruction evidence here. Native translation remains candidate, no original runtime parity.')
Path('outputs/control-tree-model-navigation-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
