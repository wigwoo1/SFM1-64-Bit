import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10353b80,0x10353b8b),(0x10353b90,0x10353b9f),(0x103905b0,0x103905b9),(0x103905c0,0x103905d5)]
groups=[[],[],[],[]];active=None
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
assert im.word(0x10b37094+119*4)==0x10353b80
Path('outputs/notification-suppression-flag-recovery.json').write_text(json.dumps(dict(complete_bytes=56,routines=groups,findings=[
 'Concrete CDataModel vtable10b37094 slot119 is10353b80: adjusts this+c118 and tailcalls103905b0.',
 'Getter103905b0 returns byte(this+74) bit4 in AL. Concrete model field is c18c, not c198.',
 'Wrapper10353b90 adjusts this+c118 then tailcalls setter103905c0. Setter uses only incoming lowbyte bit0 to replace field bit4, preserving all other bits; nonzero2 clears it.',
 'Attribute preparation1035e3a0 and element propagation1035e400 consume service slot119 AL as suppression gate. Concrete vtable mapping verified; runtime global10e9a51c instance binding and semantic flag name still need evidence.'
],limitations='Byte/vtable verification only; semantic name, actual global instance registration, original execution and production UI wiring pending.'),indent=2)+'\n')
print('Verified suppression flag wrappers/helpers bytes:',56)
