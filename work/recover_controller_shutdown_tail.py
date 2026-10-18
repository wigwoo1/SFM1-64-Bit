import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x102f1700,0x102f1731),(0x10400e50,0x10400e8e)]
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
result=dict(complete_code_bytes=111,routines=groups,findings=[
 '102f1700: nonnull global10f32e28 pool invokes vslot5(ffffffff), reloads live global pointer for importedvstdlib DestroyThreadPool, clears global, tails10400e50 even absent.',
 '10400e50: zero refcount10f4c830 no-op; decrement raw32 nonzero and cleanup only on resultingzero. Calls10402c90 then LIVEflags10f4c834 bit1=>104027f0, livebit2=>10400d50, clearsfullflagsword.',
 'Pool vslot5 semantic name and subsystem cleanup helper identities remain pending.'
],limitations='Candidate adapter and import proof; no real thread pool stopped or destroyed, production shutdown integration pending.')
Path('outputs/controller-shutdown-tail-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified controller thread pool/subsystem shutdown:',111,'bytes')
