import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x104bba30,0x104bba76),(0x104bba80,0x104bbacb)]
groups=[[],[]];active=None
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
Path('outputs/clip-time-frame-bounds-recovery.json').write_text(json.dumps(dict(complete_bytes=145,routines=groups,findings=[
 'Start104bba30 resolves clip78 time-frame handle with modelslot18; frame5c raw32-bitstart, missingframe returns0 through output argument.',
 'End104bba80 separately resolves currentclip78; adds frame64 duration to frame5c start using x86 ADD modulo32bits, missingframe returns0.',
 'No float or frame-index conversion; trackselection compares signedreturned values. Nativecandidate uses uint32 addition then bit-copy signed value to preserve wrap.'
],limitations='Complete bytes verified; time units/frame class identity/model runtime binding/original execution and native editor integration pending.'),indent=2)+'\n')
print('Verified clip time frame bounds bytes:',145)
