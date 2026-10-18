import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1097d6e0,0x1097d737)]
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
result=dict(complete_code_bytes=87,routines=groups,findings=[
 'Read checker returns false for existing error bit2. Signed maxGet1c compared to raw32 wrapped get0c+request; overflow sets bit2 and returns false.',
 'Resident range uses signed get>=offset20 and capacity4>=wrapped(get-offset+request); otherwise calls callback24(request). False callback ORs live error bit2; true callback returns success.',
 'Does not advance read cursor or recheck range after callback. Negative requests are not rejected by this body.'
],limitations='Overflow callback implementation and production parser integration pending.')
Path('outputs/buffer-read-check-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified buffer read checker:',87,'bytes')
