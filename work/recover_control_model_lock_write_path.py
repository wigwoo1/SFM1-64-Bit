import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x103fbfa0,0x103fc0e9),(0x104c2ba0,0x104c2c91)]
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
'Writer104c2ba0: null parent or both booleansfalse invokes10361280 for overrideParent,overridePos,overrideRot then returns. Active case converts parent to elementhandle10335830; looks up or creates overrideParent(type1), writes10366f90 and ORs attribute flags1000. Looks up or creates both bool attrs(type4), writes10366260. Allocation failures can leave partial attributes; no transaction rollback in this body.',
'Wrapper103fbfa0 rejects null receiver, missing104c27d0 result, or true104c58e0(receiver,parent). Both booleansfalse normalize parentnull. Collects auxiliary outputs103f9cf0, receiver and parent traversal104c4370, then calls103f9350 and103fb720 before attribute setter104c2ba0. Surrounds setter with104c5870 snapshot-like output,103fb9b0 and104c6e80 restoration-like call; frees three vectors.',
'Transform preservation is a hypothesis from surrounding calls, not proven arithmetic or animation parity. Callees must be recovered before translating wrapper.'
],limitations='First-line prefixes verified against installed image. Attribute removal/creation and typed write helper semantic names inferred from direct argument patterns; complete callee validation pending. No wrapper runtime or transform-preservation parity claim.')
Path('outputs/control-model-lock-write-path-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
