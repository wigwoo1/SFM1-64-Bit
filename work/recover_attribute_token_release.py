import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x10356e70,0x10356ed0),(0x10359c80,0x10359c9b),(0x10362c10,0x10362c2b)]
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
    '10362c10 passes cached attribute0c token to10359c80 then resets cache toffffffff.',
    '10359c80 ignoresffffffff, otherwise passes to pool managerC0ec release10356e70.',
    '10356e70 uses token&3fffff and unsigned index bound pool10; does not compare token generation. Slot state low31 increments modulo2^31, preserving high bit.',
    'If high bit not already set, sets80000000 and decrements pool0 once. Clears slot payload. If updated low31<1ff queues slot index via10354c10(pool18,&index); at threshold or above does not queue.',
    'Repeated release still increments counter and can enqueue same index while below threshold; no deduplication/generation guard appears in this body. Caller lifetime invariants unresolved.'
],limitations='First-line prefixes checked against original PE. Queue enqueue implementation, original runtime caller invariants, exceptional generation exhaustion and full native pool integration unresolved.')
Path('outputs/attribute-token-release-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
