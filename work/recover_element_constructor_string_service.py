import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1035adc0,0x1035addb)]
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
assert im.word(0x10b37094+0xc8)==0x1035adc0
assert im.string(0x10a8e744)=='name'
result=dict(complete_code_bytes=27,routines=groups,findings=[
 'CDataModel vslot50 wrapper1035adc0 forwards caller output pointer/string to1035a9d0 with model+68 manager, returns output pointer.',
 '10361c40 invokes service first with factoryname toelement34 then withconstructorarg3 into stack symbol; name attribute creation103615e0 uses installed name string,type5 andelement14 data, storesattribute pointer18.',
 '10366100 writes stacksymbol into nameattribute via update logic; ORattribute flag400 then fixed16copy completes constructor. Full update helper still pending.',
 'Symbols remain raw32; pointer widening applies to name attribute pointer rather than symbol values.'
],limitations='Full interner/attribute creation/update composition, runtime model binding and native object layout pending.')
Path('outputs/element-constructor-string-service-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified constructor string service:',27,'bytes; attribute name/type5')
