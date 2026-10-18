import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x103623b0,0x10362421)]
groups=[[]];active=None
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
result=dict(complete_code_bytes=113,routines=groups,findings=[
 '103623b0 initialflags20 rejectswithoutphasequery. Phase global10e9ccf8 slot8:0/1/2allowed;4allowedonly flags400clear;others reject.',
 'Allowed path rereadsflags; if80clear and2000set invokesowner1035e3a0(attribute). Then ORflags4080 and callsowner1035db90, returnstrue.',
 'Preparation can mutate flags; finalOR is live memory rather than captured flags. Existing marks do not bypass phase gating.'
],limitations='Complete bytes verified; phase semantic names/runtime phasebinding/owner service integration pending.')
Path('outputs/attribute-write-gate-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified attribute write gate:',113,'bytes')
