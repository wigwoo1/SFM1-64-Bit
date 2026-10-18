import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10128f50, 0x10129149)]
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

receipt=dict(verified_prefixes=spans,handler='SetOverlayInClipsWithDialogAccepted()',entry='0x10128f50',label=im.string(0x10abafd0),
    findings=[
    'Collects target clips through10128c50(document,outVector,true). Zero targets exits after freeing vector without normal picker cleanup.',
    'Picker selection count from virtual118 must be positive; otherwise same early exit.',
    'Begins data-model operation with recovered label and temporarily changes document state through10277760/10059270; semantics unresolved.',
    'Loop fetches selection at picker indexedi through virtual11c, converts QString toAscii and copies to1024-byte local buffer.',
    'Nonempty names apply via104bf960 to target vector at separate assignment index; empty names do not advance assignment index.',
    'Observed fetch-loop bound is target-clip countesi, not stored picker-count. Do not silently substitute selection count.',
    'After fetch loop, remaining targets receive the last buffer contents, including empty input if last fetched name was empty.',
    'Tail resolves a track through current-element virtual5c and begins additional insertion calculations; not yet recovered.'
    ],limitations='Bounded prefix only, through10129143; remaining handler must be recovered. Instruction prefixes checked, not all continuation bytes. Clip-selection collector, document state and actual picker index behavior pending runtime proof.')
Path('outputs/overlay-existing-clips-acceptance-prefix-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans),'label:',receipt['label'])
