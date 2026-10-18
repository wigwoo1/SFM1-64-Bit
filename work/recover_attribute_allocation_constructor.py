import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10363350,0x103633d6),(0x10365040,0x10365077)]
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
result=dict(complete_code_bytes=189,routines=groups,findings=[
 '10365040 zero type returnsnull; otherwise allocoriginal18bytes pool10f49cac, allocationfailure null, constructor10363350(owner,type,name,data).',
 'Constructor stores caller data4 and owner8, interns name->symbol14, stores type low16 into flags10, handle0c ffffffff,next0 null. Originalinputtype exactly1 or15 OR400.',
 'Calls handler table10f49bd0 indexed flagslow5 vslot2 with callerdata; afterwards ORflags200 and returnsattribute. Does not allocate data here.',
],limitations='Handler semantics/dataownership/native allocation layout/valid type range and integration pending.')
Path('outputs/attribute-allocation-constructor-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified attribute allocation/constructor:',189,'bytes')
