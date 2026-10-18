import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1097da10,0x1097da56)]
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
result=dict(complete_code_bytes=70,routines=groups,findings=[
 '1097da10 rejects error bit1 or flags bit8. Signed put>=offset and capacity>=wrapped(put-offset+request) succeeds; otherwise write callback28(request). False callback ORs live error1; true has no recheck.',
 'Composed terminator1097da60 reloads live put/offset/base after write callback. Failure clears error1 then records live put as maxGet; preexisting error1 is preserved.'
],limitations='Default write-overflow implementation pending; native adapter composition is not production CUtlBuffer integration.')
Path('outputs/buffer-write-check-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified checked write:',70,'bytes')
