import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10361790,0x103617a0),(0x10a6a580,0x10a6a5a5)]
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
vt=0x10b37a14;col=im.word(vt-4);desc=im.word(col+12)
name=im.string(desc+8)
assert name=='.?AV?$CDmAttributeOp@VCUtlSymbolLarge@@@@' and im.word(vt+8)==0x10361790
result=dict(complete_code_bytes=53,routines=groups,rtti=name,vtable=hex(vt),findings=[
 '10a6a580 allocates4byte handler, assigns vtable10b37a14 and stores pointer in type5 table10f49be4; allocationfailure writesnull.',
 'Handler RTTI is CDmAttributeOp<CUtlSymbolLarge>, initializer vslot2 at10361790 writes raw32ffffffff into supplieddata.',
 'Base constructor nameattribute uses element14 storage initialized to sentinel before assigning name symbol through10366100.'
],limitations='Original runtime registration/handler allocation native size/updatehelper/shared symbol services and full constructor integration pending.')
Path('outputs/string-attribute-handler-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified string handler registration/initializer:',53,'bytes',name)
