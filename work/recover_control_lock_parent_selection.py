import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104c3ed0,0x104c3f55),(0x104c3f60,0x104c3fe5)]
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

receipt=dict(verified_prefixes=spans,fallback_relation=im.string(0x10a8a7d4),findings=[
'Position parent selector104c3ed0 checks byte64==0, overridePos attribute present andtrue, then resolves overrideParent101e3170. Nonnull override parent returned; otherwise fallback10223170(receiver,interned relation,0,0).',
'Rotation parent selector104c3f60 identical except overrideRot. Both share relation token10f5eac4 initialized from string10a8a7d4.',
'Combined with104c5750: equal resolved parent pointers compose parent times local affine. Different pointers use rotation parent for basis concatenation, but translation is local position transformed by position parent. Null pointers supply identity.'
],limitations='First-line instruction prefixes and relation string verified. Receiver byte64 meaning and fallback10223170 precise relation semantics remain unresolved. No complete animation or runtime parity claim.')
Path('outputs/control-lock-parent-selection-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print('fallback relation:',receipt['fallback_relation'])
