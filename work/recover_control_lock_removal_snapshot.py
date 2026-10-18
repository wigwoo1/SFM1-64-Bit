import re, json
from pathlib import Path
from pe_image import Image

im = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions = [(0x10361280,0x10361386),(0x104c5870,0x104c58d3)]
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
'Remove helper10361280 interns name via data-model virtualc8, searches singly linked attr list receiver30 comparing name token at attr14. Missing name is no-op. Attribute flags200 cause no-op.',
'Allowed removal invokes10360990/10362a10, marks receiver3e bits3, optional manager1038f520, queries data-model virtualf8. If true allocates24-byte object1035e070 and submitsvirtual13c; otherwise releases detached attr1038b4f0. Unlinks attr, emits1035e400(3), ORs manager flag1000. Exact undo/event interface names unresolved.',
'Therefore lock writer clear branch attempts removal of three attrs; it is not unconditional clearing for protected attrs.',
'Snapshot-like104c5870 resolves receiver6c via data-model virtual48; resolved element calls104d2860 into48-byte temporary, missing element uses104990b0 fallback; passes temporary/output to104c5750. Transform arithmetic and output semantics need callee recovery.'
],limitations='First-line instruction prefixes checked. Data-model interface/helper names inferred only where explicitly described as hypotheses. No complete undo semantics or original-runtime/transform parity claim.')
Path('outputs/control-lock-removal-snapshot-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
old=Path('outputs/control-model-lock-write-path-recovery.json')
r=json.loads(old.read_text());r['clarification']='Clear branch attempts guarded attribute removal;10361280 preserves attributes with flags200. See control-lock-removal-snapshot-recovery.json.';old.write_text(json.dumps(r,indent=2))
