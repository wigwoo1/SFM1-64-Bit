import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10362e50,0x10362e5d),(0x103630a0,0x103630b1)]
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
result=dict(complete_code_bytes=30,routines=groups,findings=[
 '10362e50 returns AL=(attribute flags10 low5==1); identical predicate to preceding10002780 direct-data branch.',
 'With stable flags, scratch branch is unreachable for mismatched types. Separate flag reread must be preserved if mutation occurs.',
 '103630a0 copies exactly4bytes from pointed data atattribute4 into supplied output; no parsing/coercion.',
],limitations='Complete bytes verified; static initialization/global type token and native integration pending. No concurrent mutation policy inferred.')
Path('outputs/element-handle-type-check-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified handle type check/copy bytes:',30)
