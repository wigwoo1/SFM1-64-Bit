import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=0x1035e3fe:break
    if va<0x1035e3a0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035e3a0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035e3fe
Path('outputs/attribute-callback-prepare-recovery.json').write_text(json.dumps(dict(complete_bytes=94,instructions=spans,findings=[
 'Service10e9a51c slot119 byte nonzero suppresses all work. Otherwise scans retained element48 linkedrecords callback0/value4/next8.',
 'Every record whose value4 exactly equals supplied attribute pointer is unlinked in-place and prepended to element4c, reversing matching-node order while retaining unmatched order.',
 'Always enqueues elementhandle1c through1038f5e0 after scan, even when no record matched; no deduplication here.',
 'Connects attribute103623b0 prepare branch to postupdate list4c collection, proving paired value is attribute pointer on this path.'
],limitations='Complete bytes verified. Service suppression meaning/queue consumer timing/node registration/lifetime/other payload origins/original execution pending.'),indent=2)+'\n')
print('Verified attribute callback prepare bytes:',94)
