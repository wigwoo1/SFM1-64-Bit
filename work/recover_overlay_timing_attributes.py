import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104bc580, 0x104bc59b), (0x104fa730, 0x104fa7f6)]
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

receipt = dict(verified_prefixes=spans,
    attributes=[dict(name=im.string(va), value_offset=hex(offset), type_argument=kind)
        for va, offset, kind in [(0x10ab474c,0x5c,7),(0x10ab4754,0x64,7),(0x10b90cf0,0x6c,7),(0x10aa7864,0x74,3)]],
    clip_handle=dict(name=im.string(0x10b56b14), offset='0x78'),
    defaults=dict(start=0,duration=0,offset=0,scale_constant_word=hex(im.word(0x10a8a56c))),
    findings='Attribute construction independently binds timing fields to start/duration/offset/scale. The clip+78 wrapper is named timeFrame. Timing getters therefore read start and start+duration; ordering uses start.',
    limitations='Instruction prefixes verified, continuation bytes not comprehensively covered. Attribute type arguments are raw numeric values. No native UI integration or live parity claim.')
Path('outputs/overlay-timing-attribute-recovery.json').write_text(json.dumps(receipt, indent=2))
print(json.dumps(dict(attributes=receipt['attributes'],clip_handle=receipt['clip_handle'],defaults=receipt['defaults'],verified_prefix_count=len(spans)),indent=2))
