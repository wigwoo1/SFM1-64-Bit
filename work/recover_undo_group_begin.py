import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x103906f0,0x10390788)]
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
result=dict(complete_code_bytes=152,routines=groups,findings=[
 'Recording bit3 clear exits without logging or depth mutation. Otherwise logs then reloads and increments raw depth54.',
 'Only previous depth zero shifts78 to7c, stores third input78, interns first label to5c and second to60; identical input POINTER reuses symbol without second intern.',
 'Sets live pending marker bit0 then zeros submission counter70. Nested calls only increment depth.'
],limitations='Depth end protocol and production binding pending; label symbols must be native pointer width.')
Path('outputs/undo-group-begin-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo group begin:',152,'bytes')
