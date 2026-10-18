import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x102a5ae0, 0x102a5dc6)]
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

receipt=dict(verified_prefixes=spans,entry='0x102a5ae0',name=im.string(0x10ae5c9c),message=im.string(0x10ace970),
    findings=[
    'Nonzero suppression byte48 exits after profiling close.',
    'Unsuppressed path detects repeated notifications within the same current time value and emits optimization diagnostic.',
    'Traverses recipient array34/count40 backward. Resolves recipient through10996cf0; unresolved entries are removed by compaction/count decrement.',
    'Creates SelectedClipChanged message. Recipient virtualslot0 is called with message and0, return passed to global10f4c7e0 virtualslota8. Message released through10978730.',
    'Optional profiling invokes recipient virtualslot50 with elapsed milliseconds for recipient name/diagnostic.',
    'Obtains controller100a6250, helper105c2e60; ifnonnull collects selected film clips using10128c50(document,out,true). Exactlyone target invokes10095cf0(helper,clip,0).',
    'Temporary collection destroyed; profiling closed. Observed document handlearrayb8/countc4 is the collection used for this selection notification.'
    ],limitations='Entire function first-line prefixes checked, continuation bytes not comprehensively verified. Recipient/bus interface identity, getter105c2e60 and helper10095cf0 effects pending. No direct Qt slot link or complete editor notification parity established.')
Path('outputs/movie-document-selection-notification-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans),'message:',receipt['message'])
