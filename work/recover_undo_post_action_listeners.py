import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10353b50,0x10353b5b),(0x103908b0,0x103908d5)]
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
assert im.word(0x10b37094+126*4)==0x10353b50
result=dict(complete_code_bytes=48,routines=groups,findings=[
 'Model slot126 forwards this+c118 to103908b0.',
 'Snapshots signed listener count48; iterates ascending indices below snapshot count, reloads live listener vector3c each iteration and invokes each listener virtual slot0 without arguments.',
 'No null guard, duplicate filtering or recording-flag gate exists in this hook. Actual listener callback implementations remain external.'
],limitations='Listener registration/lifetime and production UI refresh implementation pending.')
Path('outputs/undo-post-action-listeners-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo post-action listeners:',48,'bytes')
