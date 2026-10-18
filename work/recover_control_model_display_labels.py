import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1007f3a0,0x1007f553),(0x1008d3b0,0x1008d40c)]
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

labels={str(k):dict(pointer=hex(im.word(0x10e7f764+4*k)),text=im.string(im.word(0x10e7f764+4*k))) for k in range(5,13)}
formats={hex(a):im.string(a) for a in [0x10aa5274,0x10aa527c,0x10aa5288,0x10aa5294,0x10aa525c,0x10aa7520]}
receipt=dict(verified_prefixes=spans,kind_labels=labels,formats=formats,findings=[
'Node name getter1008d3b0 resolves group handle+c for kinds<4, shared controlrecord handle forleaf>=4. Unresolved element returns placeholderstring10aa7520. Resolved kind>4 uses staticlabeltable; kinds<=4 use elementname14 or emptystring ifinvalid.',
'Display wrapper1007f3a0 obtains base name and nodeelement; can augment with metadata formatting for typeglobal10eb1ac4 and active transform target. Default returns base name as QVariant(charconst*). Full augmentation helper identities pending.'
],limitations='First-line prefixes and table strings checked. Known kinds5..12 mapped independently; no claim completekinddomain. Display augmentation and full Qt appearance semantics/runtime/UI integration remain pending.')
Path('outputs/control-model-display-label-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print(labels);print(formats)
