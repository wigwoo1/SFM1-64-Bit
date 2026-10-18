import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x103615e0,0x10361701)]
groups=[[]];active=None
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
result=dict(complete_code_bytes=289,routines=groups,findings=[
 '103615e0 queues element1c via1038f520 if state3e bit1 clear, then ORstate3e3 before any duplicate check.',
 'Interns requested name via model50, scans live attribute list30 for symbol14 equality; duplicate returnsnull with preceding marking already performed.',
 'New path savesmodel61/sets60(0), calls10365040(owner,type,name,data), prepends returned attribute without nullcheck, restores10007370.',
 'Optional model62 change record alloc24/init1035e070 then model79; finally propagatesmask3 through1035e400, sets modelc180bit1000, calls10360800(owner,newattribute).'
],limitations='Complete bytes verified; allocation/helper semantics/runtimebinding/full native creation pending; scope60/61 is separate from notification119/120.')
Path('outputs/element-attribute-creation-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified attribute creation body:',289,'bytes')
