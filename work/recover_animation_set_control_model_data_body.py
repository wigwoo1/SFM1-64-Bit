import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x1007f560,0x1007fd10)]
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

def switchmap(bytebase,n,targetbase):
 values=im.b[im.offset(bytebase):im.offset(bytebase)+n]
 return {str(i):hex(im.word(targetbase+4*v)) for i,v in enumerate(values)}
rolemap=switchmap(0x1007fd40,10,0x1007fd2c)
labels={hex(a):im.string(a) for a in [0x10aa5298,0x10aa52c0,0x10aa52e8,0x10aa5310,0x10aa5350]}
receipt=dict(verified_prefixes=spans,role_dispatch=rolemap,referenced_strings=labels,findings=[
'Data routine1007f560 nullnode returns emptyvariant. Columns2/3 special-case role10 checkbox data. Column1 has distinct role10 lock state kind dispatch. Other data roles dispatch throughbyte/dword tables outside functionbody.',
'Selection appearance branch1007f8c9 uses recovered raw selection state:2 produces one palette/brush path,3 uses alternatecolor-related path; other states emptyvariant. Actual Qt imports must be independently identified before naming colors/roles.',
'Visible labels and icons route through several helpers; full data routine prefixes saved to enable targeted recovery without relying on filtered output.'
],limitations='First-line prefixes checked; full branch semantics not yet decompiled. Selected strings recorded as references, not guessed kind labels. No full UI data-role or runtime parity claim.')
Path('outputs/animation-set-control-model-data-body-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print(rolemap);print(labels)
