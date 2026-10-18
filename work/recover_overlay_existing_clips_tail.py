import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10129149, 0x1012946d), (0x1097ae30, 0x1097aea3)]
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

receipt=dict(verified_prefixes=spans,handler='SetOverlayInClipsWithDialogAccepted()',
    findings=[
    'Extra count=max(pickerCount-targetCount,0). Last target end and duration saved.',
    'Calls1097ae30 on duration with float(extraCount+1). Helper performs signed-int-to-float division, rounding sequence, writes result to input and output.',
    'Sets last target duration to divided duration; extra insertion starts at last target start+divided duration.',
    'Loops picker indices from targetCount through pickerCount-1; creates typed film clips with Default Set and material assignments, inserts into resolved track and calls102a6020(document,true,newclip).',
    'New clip durations use divided duration except final extra, whose duration equals original end minus insertion start. Insertion start advances by divided duration.',
    'Calls102a32e0 with track; restores saved document state through10059270; closes data-model scopes; calls102a5ae0; picker virtual0xc(true); parent window raise/activate; QObjectdeleteLater; frees temporary target vector.'
    ],division_constants={hex(va):hex(im.word(va)) for va in [0x10a99998,0x10aac1ac,0x10a8a56c,0x10a8a87c]},
    limitations='Tail plus division helper instruction prefixes checked, not all continuation bytes. Exact rounding/native parity and document mutation/selection call semantics remain unresolved. Paired with acceptance-prefix receipt, not a completed native handler.')
Path('outputs/overlay-existing-clips-acceptance-tail-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
