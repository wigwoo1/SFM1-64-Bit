import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10222de0,0x10222e76),(0x10223170,0x102231f9)]
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

dispatch=[hex(im.word(0x10222e78+4*i)) for i in range(4)]
receipt=dict(verified_prefixes=spans,mode_dispatch=dispatch,findings=[
'10223170 converts element tohandle, obtains first incoming reference via data-model virtual1cc; stores element38 as comparison token. Walks references with virtual1d0 until10222de0 returns owning element, else returnsnull.',
'Checker10222de0 resolves reference attribute via virtual164, uses mode switch0..3. Lock fallback mode0 jumps directly to owner retrieval at10222e33, skipping attribute flags800/1000 filtering.',
'Owner attr8 requirednonnull and virtual10 type test against global10eb131c must succeed. Requested name token compared with attribute14 unlessffffffff wildcard. Optional same-token restriction compares owner38 to originalelement38; lock fallback passesfalse.',
'Lock parent fallback therefore searches incoming references named children, returning first qualifying typed owner, not a forward traversal of the receiver children array.'
],limitations='First-line instruction prefixes checked, switch table decoded as data. Typeglobal10eb131c identity, exact data-model ref interfaces and element38 semantics unresolved. No complete runtime selection parity claim.')
Path('outputs/control-lock-parent-reference-resolution.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print(dispatch)
