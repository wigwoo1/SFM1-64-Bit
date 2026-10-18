import json, re
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104cbd50, 0x104cbe1b), (0x104cbe30, 0x104cbe6f), (0x104cbe90, 0x104cbf25)]
spans = []
for line in Path('work/ifm-disassembly.txt').open():
    m = re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)', line)
    if not m: continue
    va = int(m[1], 16)
    if va >= regions[-1][1]: break
    if not any(a <= va < b for a,b in regions): continue
    raw = bytes.fromhex(m[2]); at = im.offset(va)
    assert im.b[at:at+len(raw)] == raw
    spans.append(dict(va=hex(va), bytes=raw.hex(), instruction=m[3].strip()))
bindings = []
for address, expected, offset, attr, dtype in [
    (0x10aa55f8, 'valuePosition', 0x6c, 0x78, 0x0a),
    (0x10aa55e4, 'valueOrientation', 0x7c, 0x8c, 0x0d),
    (0x10aa5a9c, 'positionChannel', 0x90, 0x98, 1),
    (0x10aa5aac, 'orientationChannel', 0x9c, 0xa4, 1)]:
    assert im.string(address) == expected
    bindings.append(dict(name=expected, string_va=hex(address), wrapper_offset=hex(offset), attribute_pointer_offset=hex(attr), type_code=dtype))
receipt = dict(verified_prefixes=spans, bindings=bindings,
    findings=[
        '104cbd50 binds four wrappers via103615e0. Value defaults are copied from global position/orientation data.',
        'Both channel attributes are type1 and passed global10eb17c4 to10363530; exact helper/type constraint semantics pending.',
        '104cbe30/104cbe50 resolve position/orientation channel handles through datamodel virtual48, return null on failure, otherwise subtract4.',
        '104cbe90/104cbee0 channel setters obtain nonnull input handle from input+4 via10335830; null writes ffffffff; both write through10366f90.'
    ], limitations='First-line instruction prefixes checked against installed PE. Continuation bytes, binding helper implementation, runtime callbacks and complete native graph integration are unverified. No full decompilation claim.')
Path('outputs/transform-control-channel-bindings-recovery.json').write_text(json.dumps(receipt, indent=2))
print('Verified prefixes:', len(spans)); print(bindings)
