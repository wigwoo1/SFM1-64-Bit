import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10353bf0,0x10353bfc),(0x10353c00,0x10353c2d),(0x10353d70,0x10353d7f)]
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
assert im.word(0x10b37094+0xf4)==0x10353bf0
assert im.word(0x10b37094+0xf8)==0x10353c00
assert im.word(0x10b37094+0x13c)==0x10353d70
result=dict(complete_code_bytes=72,routines=groups,findings=[
 'Model61 getterreturnsmodelc18c bit3. Model62 queriesvirtual61 FIRST, then rejectsnullowner orowner38ffffffff; otherwise recordsallowed.',
 'Model79 submitadjuststhis+c118 andtailcalls10390ce0(record). Actualqueue/null/lifetime behavior pending.',
 'Recordenabled bit3 differsfromnotification suppressionbit4 onsharedmodelbytec18c.'
],limitations='Completebytesverified; enable setter/replay toggles/owner38semantics/undoqueue submission and runtime integration pending.')
Path('outputs/model-undo-record-gate-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo record gate/getter/submit wrapper:',72,'bytes')
