import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=0x1035a435:break
    if va<0x1035a2e0:continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
report=dict(verified_prefixes=spans,findings=[
 'CDataModel slot93 normalizes name again via10353980 then symbol-find10359ca0 on manager68; FFFFFFFF symbol returns immediately.',
 'Captures signed pool countc1ac; nonpositive returnsFFFFFFFF. Captures tablec1a0 and scans slots ascending; skips initial null pointers.',
 'Constructs handle from low11 entry generation shifted20 and low20 slot index, rejects unrepresentable slot andFFFFFFFF.',
 'Resolves handle against current manager table/count/generation/disabled flags; compares resolved object0 name symbol; first match returns constructed handle.',
 'Lookup uses symbol identity, not direct case-insensitive string comparison or UUID identity.'
],limitations='Instruction prefixes verified. Symbol-find internals10359ca0, full continuation bytes, pool mutation callbacks, native execution parity and session owner assignment remain unverified.')
Path('outputs/element-owner-lookup-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified owner name lookup prefixes:',len(spans))
