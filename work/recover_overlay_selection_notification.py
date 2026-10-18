import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10059270, 0x1005927d), (0x10277760, 0x10277764), (0x102a5ae0, 0x102a5b21)]
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

receipt=dict(verified_prefixes=spans,document_flag_offset='0x48',getter='0x10277760',setter='0x10059270',
    notification='0x102a5ae0',profiling_label=im.string(0x10ae5c9c),diagnostic=im.string(0x10ae5c38),
    findings='Getter/setter read/write byte48. NotifySelectionChanged entry opens profiling scope and immediately returns when byte48 is nonzero. Change Overlays captures old byte, writes1 before edits, restores original byte and then calls notification; nested suppression therefore remains effective.',
    limitations='Notification guard only, not full observer dispatch. Setter shared code may serve additional classes. Raw-byte preservation is required; replacing restore with unconditionalfalse is incorrect. Native integration/runtime notification parity pending.')
Path('outputs/overlay-selection-notification-suppression-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans),'notification:',receipt['profiling_label'])
