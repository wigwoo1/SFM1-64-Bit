import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x103596d0,0x103597de),(0x1035dc10,0x1035dc14)]
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=regions[-1][1]:break
    if not any(a<=va<b for a,b in regions):continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
    '103596d0 second argument is optional saved-record output pointer. Nonnull output is initialized to null before live lookup.',
    'Live lookup via manager virtual48: resolved object returns object+1c via1035dc10, leaving optional output null.',
    'On live resolution failure, scans managerC1ec array of20-byte chunk descriptors, signed countC1f8. Descriptor0 points to36-byte records, descriptor0c is signed record count.',
    'Iteration begins first nonempty chunk and visits records in chunk/index order using encoded upper16 chunk/lower16 index. Compares complete requested handle with record10. No standalone generation extraction is performed here.',
    'First exact match returns record+10, and if output pointer supplied stores record base. Missing match returns null. This preserves reference state even without a resolved live object.',
    'Manager virtual48 generation/validity rules and saved-record lifecycle remain pending.'
],limitations='First-line instruction prefixes verified against installed PE. Continuation-byte verification, runtime manager layout, chunk overflow invariants and native graph parity unresolved.')
Path('outputs/incoming-reference-record-lookup-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
