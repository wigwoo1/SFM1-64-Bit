import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x104c3e30,0x104c3ec3)]
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

labels={hex(a):im.string(a) for a in [0x10b8e468,0x10b8e494,0x10b8e488]}
assert list(labels.values())==['overrideParent','overridePos','overrideRot']
receipt=dict(verified_prefixes=spans,entry='0x104c3e30',attributes=labels,findings=[
'Initializes both output booleans false. If receiver byte64 nonzero and third argument false, returnsnull without attribute lookup.',
'Helper101e3170 receives overrideParent string; its result supplies candidate parent return. Only if nonnull does function look up overridePos and overrideRot via1035dd50 and read booleans via10058790, missing attrs defaultfalse.',
'First output is overridePos, second output overrideRot. Returnsnull if both false; otherwise returns candidate override parent.',
'Combined with column1 edit routine: kind4 toggles both, kind5 toggles position only, kind9 toggles rotation only. These are actual attribute meanings; visible child display labels remain separately unverified.'
],limitations='First-line instruction prefixes and attribute strings verified against installed DLL. Receiver byte64 meaning, overrideParent helper semantics, writer103fbfa0 full behavior, and original runtime parity unresolved.')
Path('outputs/control-model-lock-attributes-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans));print(labels)
