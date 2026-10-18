import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104bd2f0, 0x104bd32b), (0x105dd0a0, 0x105dd0c1)]
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
    film_clip_attribute=dict(name=im.string(0x10ae4404),handle_offset='0x144',attribute_pointer_offset='0x14c',type_argument=1),
    overlay_effect_attribute=dict(name=im.string(0x10abd754),value_offset='0xa4',attribute_pointer_offset='0xa8',type_argument=5),
    findings='Film clip materialOverlay is an element-reference attribute. Effect material attribute is constructed at+a4/a8 with raw type5. Empty-input setter clears the materialOverlay element reference through its attribute pointer+14c; it does not clear the effect material string.',
    limitations='Prefixes verified; raw numeric type arguments recorded. Full construction and underlying setter/string conversion semantics still pending. No native integration or runtime parity claim.')
Path('outputs/overlay-material-attributes-recovery.json').write_text(json.dumps(receipt,indent=2))
print(json.dumps({k:v for k,v in receipt.items() if k != 'verified_prefixes'},indent=2))
