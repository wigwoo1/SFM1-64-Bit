import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104c6ad0,0x104c6e71)]
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
'104c6ad0 selects position/rotation parents. Different parents:104c5a00 obtains both matrices,10498630 copies each,1007fdf0 returns scalar, square scalar then compute1/square, multiply nine basis components unless exactly1.0. NaN also follows multiply branch.',
'Adjusted matrices pass1049d6d0 into separate outputs. Concatenate position adjusted output times inputworld into temp; rotation adjusted output times inputworld into final; overwrite final translation with position temporary translation. Equalparents executes one adjustment/matrixhelper/concatenation path.',
'This is exact call/data flow;1049d6d0 inverse semantics and1007fdf0 scalar meaning not yet independently recovered. Do not replace with generic4x4 inverse before verifying helper.'
],limitations='First-line instruction prefixes checked. Helpers104c5a00/10498630/1007fdf0/1049d6d0 semantics still pending, including singular/nonuniform scale behavior. Full original runtime/animation/UI parity remains unverified.')
Path('outputs/control-lock-world-to-local-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
