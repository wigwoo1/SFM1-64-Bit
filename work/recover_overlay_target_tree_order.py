import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10127d20, 0x10127e0c), (0x10128520, 0x101285b9), (0x101285c0, 0x10128729)]
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

receipt=dict(verified_prefixes=spans,
    findings=[
    'Insertion search10127d20 starts tree root+10; compares candidate payload against node payload through tree comparator.',
    'Strict-less=true takes child at node+0 and sets output left flagtrue; false takes child at node+4 and sets flagfalse. Equal starts therefore take right-child path, not duplicate rejection.',
    'First iterator10128520 follows node+0 links from root until absent.',
    'Next iterator101285c0 follows right subtree then leftmost descendant when right child exists; otherwise climbs parents, using10127c70 to decide whether current node is a right child.',
    'All comparator-equal clips are retained as distinct nodes by collector; complete final tie order requires rebalancing and parent-helper proof.'
    ],limitations='Insertion search and iterator instruction prefixes checked. Rebalancing101287f0 and child/parent helpers not fully recovered; do not yet substitute stable_sort or claim exact equal-start output order. Runtime parity pending.')
Path('outputs/overlay-target-tree-order-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
