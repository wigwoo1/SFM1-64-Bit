import json, re, hashlib
from pathlib import Path
from pe_image import Image

source = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
im = Image(source)
regions = [(0x101281e0, 0x10128520), (0x10127b50, 0x10127bae),
           (0x104bf960, 0x104bf9e7)]
spans = []
for line in Path('work/ifm-disassembly.txt').open():
    m = re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)', line)
    if not m:
        continue
    va = int(m[1], 16)
    if va >= max(end for _, end in regions):
        break
    if not any(start <= va < end for start, end in regions):
        continue
    raw = bytes.fromhex(m[2])
    at = im.offset(va)
    assert im.b[at:at + len(raw)] == raw, hex(va)
    spans.append(dict(va=hex(va), bytes=raw.hex(), instruction=m[3].strip()))

r = dict(source=str(source), sha256=hashlib.sha256(im.b).hexdigest(),
         handler='CQVMTPickerDialogHelper::AddOverlayToTrackWithDialogAccepted()',
         entry='0x101281e0', verified_instructions=spans,
         verified_instruction_count=len(spans),
         verified_byte_count=sum(len(item['bytes']) // 2 for item in spans),
         labels={hex(va): im.string(va) for va in [0x10abafc0, 0x10abafb4, 0x10ae4404]},
         observed_flow=[
             'Selection count <= 0, missing track, or track type != 3 exits at 0x10128510.',
             'Resolves insertion start through track/current-element helper calls.',
             'Data-model calls carry the Add Overlays label before the selection loop.',
             'Each nonempty selected name invokes 0x104d3ef0 with start and start+duration.',
             'Factory 0x10127b50 uses data-model virtual slots 0x40/0x48 and type check slot 0x10.',
             'New element receives Default Set through 0x104bfbf0, start/duration through 0x104bbb60/0x104bbba0, selected name through 0x104bf960/0x104bb5b0, then 0x104d2fe0.',
             'Insertion start advances by duration after each nonempty selection.',
             'Changed-track notification is conditional on an inserted item.',
             'Normal tail releases picker via virtual slot 0x0c, performs parent operations, and invokes QObject::deleteLater.'
         ],
         limitations='Every listed disassembly instruction byte sequence was compared to the installed binary. Semantic names of transaction virtual slots, timing/overlap helpers, factory type, and material setter remain unresolved. Guarded early exits bypass the normal cleanup tail. No native UI integration or runtime parity claim.')
Path('outputs/overlay-acceptance-body-recovery.json').write_text(json.dumps(r, indent=2))
print(json.dumps(dict(verified_prefix_count=len(spans), labels=r['labels']), indent=2))
