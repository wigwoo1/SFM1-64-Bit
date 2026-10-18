import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x1035ab20,0x1035abdb),(0x1035b4c0,0x1035b519)]
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
    '1035b4c0 obtains token from pool at managerC0ec via1035ab20. If token invalid, index out of bounds or slot generation comparison fails, returns token without binding.',
    'Token index is token&3fffff. Slot size8 at pool4; count/capacity bound pool10. Generation extract(token>>22)&1ff XOR slotword must be zero under mask7fffffff.',
    'Valid slot: if high bit80000000 set, increment pool0 and clear high bit; store attribute pointer at slot4. Wrapper returns original token.',
    '1035ab20 consumes ring queue at pool18 with capacity1c, head24/tail28; one entry resets both toffffffff, otherwise head advances and wraps. Empty queue obtains new slot through10357810(pool4,pool10).',
    'Allocator clears slot high bit, zeroes payload, increments pool0. Token packs(slotword&1ff)<<22 with index&3fffff, ORs stack-local ebp-4 high bit, checks original index representability and returnsffffffff if truncated.',
    'Visible function does not explicitly initialize ebp-4 before its high-bit read (except reading it in unusual queue branch); exact compiler artifact or preceding/context semantics unresolved. Do not substitute a guessed high-bit policy.'
],limitations='First-line instruction prefixes checked against PE. Full continuation-byte decoding, slot-growth helper, queue initialization, token lifetime/generation increments, stack-local semantics and runtime parity unverified.')
Path('outputs/attribute-token-allocation-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
