import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1008d600, 0x1008d7e0)]
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

labels=[0x10aa7784,0x10aa7728,0x10aa76d0,0x10aa7670,0x10aa7620,0x10aa75d0,0x10aa7580]
receipt=dict(verified_prefixes=spans,class_evidence=im.string(0x10aa7784),profiling_labels={hex(va):im.string(va) for va in labels},
    model_imports='outputs/selected-film-editor-model-imports.json',
    findings=[
    'Rebuild10095b10 calls beginResetModel(model64) and endResetModel(model64), then UpdateSelectionState1008d600(true).',
    'UpdateSelectionState first invokes1008c190 on control-group structure1c.',
    'When member6c and its member1c exist, invokes target virtual104 with two initialized index-like local records.',
    'Traverses member90/count9c forward, invoking virtual0 on each entry.',
    'Calls1007eed0(model64,QString) and10096bb0(member68,QString) with initially shared empty-string storage.',
    'When argumenttrue, obtains target102bc6c0 from global10f32568 and calls101df990 with this+50.',
    'No direct QMetaObjectactivate import call in this recovered function.'
    ],limitations='Full update routine instruction prefixes checked, not continuation bytes. Profiling labels establish Animation Set Editor controller identity, but individual view/model method and recursive selection semantics still pending. Native UI/runtime parity unverified.')
Path('outputs/animation-set-editor-selection-update-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print(json.dumps(receipt['profiling_labels'],indent=2))
