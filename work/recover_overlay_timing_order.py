import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1017c0c0, 0x1017c0e2), (0x104bba30, 0x104bba76),
           (0x104bba80, 0x104bbacb), (0x104d34e0, 0x104d363a)]
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

receipt = dict(verified_prefixes=spans, findings=[
    'Getter104bba30 resolves clip+0x78 and copies resolved+0x5c, or zero if unresolved.',
    'Getter104bba80 resolves the same handle and copies resolved+0x5c + resolved+0x64, or zero if unresolved. Arithmetic is 32-bit machine addition.',
    'Ordering104d34e0 skips cached type3 tracks with flagA8 bit0 set. Otherwise sets bit0, constructs eight-byte timing/pointer pairs, and detects descending timing.',
    'Invalid elements receive timing INT32_MIN and a null pointer.',
    'On descending timing, invokes sort with comparator1017c0c0, comparing signed timing only: -1, 0 or +1. Equal-time order is not guaranteed by this comparator.',
    'Rebuilds handle array, preserves invalid entries as handle0xffffffff, brackets rebuilding with flagA8 bits1/2, then clears those bits.'
], limitations='First-line instruction prefixes checked; continuation bytes not comprehensively covered. Attribute names and sort implementation identity remain unresolved. Start/end interpretation supported by arithmetic and callers but pending independent attribute metadata. Runtime parity and native integration pending.')
Path('outputs/overlay-timing-order-recovery.json').write_text(json.dumps(receipt, indent=2))
print('Verified instruction prefixes:', len(spans))
