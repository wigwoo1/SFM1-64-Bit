import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]
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
result=dict(complete_code_bytes=sum(e-s for s,e in ranges),routines=groups,findings=[
 '10298ad0 looks up frameRate through1028bee0/1035dd50/100276d0, defaults24, constructs packed rate1097b900 and calls1097b020 on input time with caller mode.',
 '1097b020 first calls1097af20(input time, packed rate, caller mode); returned signed integer converts to float then double, multiplies units high16/divides numerator low16, calls1097ab80 with mode0 and stores output.',
 'Adjustment therefore includes an integer intermediate and float narrowing; it cannot be replaced by an assumed identity or direct double time scaling.'
],limitations='1097af20 arithmetic helpers and mode semantics need complete recovery; no production implementation or original execution parity claimed.')
Path('outputs/editor-time-adjustment-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified adjustment code bytes:',result['complete_code_bytes'])
