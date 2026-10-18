import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1009a210,0x1009a3a2),(0x1009d0e0,0x1009d0e8)]
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
strings={hex(v):im.string(v) for v in (0x10aa8828,0x10aa881c)}
result=dict(complete_code_bytes=410,routines=groups,strings=strings,findings=[
 '1009d0e0 ignores both caller arguments, calls1009a210 with same this, returns discarding8 argument bytes.',
 '1009a210 returns if controller field8 null; otherwise reads current global child344. Present child supplies byte state via10275f10 and title string via102787d0; absent branch supplies zero and installed text.',
 'Calls imported string composition/setter routines and reference-count release operations. Import identities and full string composition semantics remain pending.'
],limitations='Static bytes verified; Qt import identities/native UI update integration pending.')
Path('outputs/controller-followup-update-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified controller follow-up/update:',410,'bytes',strings)
