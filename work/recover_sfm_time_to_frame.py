import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1097af20,0x1097b012),(0x10a16680,0x10a1672a),(0x10a16af0,0x10a16b24)]
groups=[[],[],[]];active=None
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
result=dict(complete_code_bytes=sum(e-s for s,e in ranges),routines=groups,findings=[
 '1097af20 multiplies signed32 time by unsigned16 numerator through low64 multiply helper10a16af0; denominator is unsigned16 units.',
 'For negative time mode0 subtract units/2 then add numerator; mode1 subtract units then add numerator; other modes leave product unchanged.',
 'For nonnegative time mode0 adds units/2; mode2 subtracts numerator then adds units; modes1/3 add numerator; other modes leave product unchanged.',
 'All paths signed-divide by units via10a16680, return low32 quotient. Preserve literal adjustments rather than replacing with generic floor/ceil.'
],limitations='Complete bytes verified; zero denominator faults and original execution parity remain pending. Native candidate will restrict denominator to nonzero.')
Path('outputs/sfm-time-to-frame-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified integer conversion/helper bytes:',result['complete_code_bytes'])
