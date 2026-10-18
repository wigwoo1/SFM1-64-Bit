import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[]
active=None
ranges=[(0x10975d20,0x10975d89),(0x10975f80,0x109760c2),(0x10a64a90,0x10a64aed)]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        continuation=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if continuation and active is not None:
            active['bytes']+=bytes.fromhex(continuation[1]).hex()
        continue
    va=int(m[1],16)
    if va>=ranges[-1][1]:break
    if not any(a<=va<b for a,b in ranges):
        active=None
        continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    active=dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip())
    spans.append(active)
for a,b in ranges:
    cursor=a
    for span in spans:
        va=int(span['va'],16)
        if not a<=va<b:continue
        raw=bytes.fromhex(span['bytes'])
        assert va==cursor,(hex(va),hex(cursor))
        assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
        cursor+=len(raw)
    assert cursor==b
report=dict(verified_complete_instructions=spans,complete_routine_bytes=sum(b-a for a,b in ranges),findings=[
 '10975f80 collapses adjacent slash/backslash runs, removes dot+slash whenever dot is at start or previous byte not dot, strips trailing slash-dot when length>2, then resolves eligible dot-dot components.',
 'Dot-dot without earlier separator returns false after previous in-place cleanup; caller10976270 ignores result and continues.',
 '10975d20 begins index1, compares current/next separators, shifts suffix including terminator when duplicate, decrements length, advances index even after removal. First separator pair is not examined.',
 '10a64a90 global10fdb1c0 zero path lowercases ASCII A..Z in place, preserves other byte values; nonzero delegates10a64a5c with locale0/countFFFFFFFF.',
 'Null lowercase input invokes invalid parameter/error16 path. Candidate fast path requires nonnull source and explicit caller knowledge of zero global branch.'
],limitations='All continuation bytes and contiguous instruction coverage verified for three complete routines. Bundled memmove/strncpy/locale, dot-dot cleanup runtime execution and full owner path normalization parity unverified. Preserve cleanup order and ignored failure result; no filesystem library substitution.')
Path('outputs/element-owner-path-cleanup-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified complete path cleanup instructions:',len(spans),'bytes:',sum(b-a for a,b in ranges))
