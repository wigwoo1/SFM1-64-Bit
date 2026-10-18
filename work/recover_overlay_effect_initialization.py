import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x105dde40, 0x105ddf47), (0x105de020, 0x105de03e), (0x105de090, 0x105de0d0)]
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

assert im.word(0x10b97888)==0x105de090
receipt=dict(verified_prefixes=spans, factory_allocation_bytes=0x11c, constructor='0x105dde40',
    initial_values={'material':'0xffffffff string word','overlaycolor':'0xff000000 packed word','left':0,'top':0,'width':0,'height':0,'fullscreen':False,'useSubRect':False,'subRectLeft':0,'subRectTop':0,'subRectWidth':0,'subRectHeight':0,'movementAngle':0.0,'movementSpeed':0.0},
    post_attribute_construction_overrides={'overlaycolor':'0xffffffff packed word','fullscreen':True,'top':0,'left':0,'height':1,'width':1},
    findings='Factory105de090 allocates0x11c and calls105dde40. Constructor initializes raw wrapper values and null attribute pointers. Attribute construction105dd0a0 sets top0 then copies that value into left; sets height1 then copies it into width. Construction dispatch105de020 reaches105dd0a0.',
    limitations='Prefixes checked; continuation bytes not comprehensively checked. Complete base construction, registration lifecycle, string sentinel interpretation and runtime parity pending. Defaults describe newly constructed state, not loaded-project overrides.')
Path('outputs/overlay-effect-initialization-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified instruction prefixes:',len(spans))
