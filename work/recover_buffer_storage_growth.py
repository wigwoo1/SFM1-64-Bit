import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1025f0c0,0x1025f14c)]
groups=[[] for _ in ranges];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16);active=None
    if va>=ranges[-1][1]:break
    for i,(start,end) in enumerate(ranges):
        if start<=va<end:
            active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());groups[i].append(active);break
for (start,end),spans in zip(ranges,groups):
    cursor=start
    for s in spans:
        va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
        assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
        cursor+=len(raw)
    assert cursor==end
result=dict(complete_code_bytes=140,routines=groups,findings=[
 'Negative growSize exits. requested=wrapped(capacity+increment). Positive growSize uses signed((requested-1)/grow+1)*grow. Zero growSize starts32 if empty then doubles until signed capacity>=requested.',
 'Postcalculation signed shortfall includes zero/-1 special case and wrapped sum divided toward zero by2 loop.',
 'Snapshots old storage, publishes new capacity BEFORE allocator. Nonnull old pointer calls allocator slot3 reallocate(old,newsize), null calls slot1 allocate(newsize); stores result unconditionally.',
 'No allocation failure rollback or growth termination guard. Candidate intentionally retains original control flow; pathological integer inputs can fail to terminate.'
],limitations='Static recovery and bounded x64 fixtures; production allocator and original executable parity not verified.')
Path('outputs/buffer-storage-growth-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified storage growth:',140,'bytes')
