import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10430180,0x1043018a),(0x10453140,0x1045316d),(0x10453170,0x104531f2)]
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
calls=[int(s['instruction'].split()[-1],16) for s in groups[2] if s['instruction'].startswith(('call','jmp'))]
assert len(calls)==26
result=dict(complete_code_bytes=185,routines=groups,ordered_registration_helpers=[hex(v) for v in calls],source_string=im.string(0x10b5e56c),findings=[
 '10430180 calls10453170 then tails10453140. The first runs26 registration helpers in fixed order; final helper is a tail call.',
 '10453140 passes global10eaa7b0 value to104311e0; if nonzero result exits. Zero result registers table10eaa7b0 with argument0 and table10eaadb8 with argument0 via10431080.',
 'Source filename string at10b5e56c provides embedded-library provenance context; helper identity remains an inference until further source/binary mapping.'
],limitations='Static recovery only; inner error table registration and wider library startup remain incomplete. No production registration calls executed.')
Path('outputs/controller-registration-startup-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified startup registration sequence:',185,'bytes; source string:',result['source_string'])
