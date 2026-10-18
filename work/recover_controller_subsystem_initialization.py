import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1025eb70,0x1025eb87),(0x10400dd0,0x10400e4f)]
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
result=dict(complete_code_bytes=150,routines=groups,findings=[
 '10400dd0 increments raw32 refcount before testing previousvalue. Previousnonzero returns0 without setup. First initialization installs allocator hooks, including10ea0c34=1025eb70.',
 '1025eb70 delegates pointer argument to live allocator global10a7e484 indirection vslot5. Disposal libc free fixture does not prove original allocator equivalence.',
 'First-initflags bit1 calls104027d0 and returns2 on resultzero; bit2 calls10400d60 and returns2 on nonzero result. Failure leaves incremented refcount and does not publish requestedflags. Success writes flags, calls104025e0, returns0.'
],limitations='Allocator implementation and initialization helper bodies pending; this proves setup assignment, not immutable callback binding across all runtime writes.')
Path('outputs/controller-subsystem-initialization-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified initialization and disposal allocator provenance:',150,'bytes')
