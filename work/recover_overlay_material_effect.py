import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104be2c0, 0x104be31e), (0x104bf960, 0x104bf9e7), (0x105dcf90, 0x105dcfc2), (0x105ddde0, 0x105dddef)]
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

assert im.word(0x10b9789c) == 0x105ddde0
locator=im.word(0x10b97884);td=im.word(locator+12);name=im.string(td+8)
assert name == '.?AV?$CDmFXClipFactory@VCDmeMaterialOverlayFXClip@@@@'
receipt=dict(verified_prefixes=spans,factory_rtti=name,locator=hex(locator),type_descriptor=hex(td),type_global='0x10eb71c4',
    findings=[
    'Nonempty input resolves film clip handle+0x144; when missing, creates materialOverlay through factory104be2c0 and assigns returned element to the handle wrapper.',
    'Factory104be2c0 uses type global10eb71c4; its setter105ddde0 belongs to CDmFXClipFactory<CDmeMaterialOverlayFXClip>.',
    'Resolved effect receives selected material string through105dcf90. That method passes the string to data-model virtual0xc8 and assigns returned word via attribute pointer effect+0xa8.',
    'Null or empty input takes a separate branch assigning word0xffffffff through film clip attribute pointer+0x14c; this is not the normal material-name setter.'
    ],limitations='First-line instruction prefixes checked. Attribute names at effect+a4/a8 and film clip+144/14c unresolved. Data-model virtual0xc8 semantics not independently named. No UI/native integration or runtime parity claim.')
Path('outputs/overlay-material-effect-recovery.json').write_text(json.dumps(receipt,indent=2))
print(json.dumps(dict(factory_rtti=name,verified_prefix_count=len(spans)),indent=2))
