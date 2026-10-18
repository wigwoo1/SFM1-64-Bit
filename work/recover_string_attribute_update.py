import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1035eff0,0x1035f008),(0x10365ba0,0x10365c00),(0x10366100,0x103661c9)]
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
result=dict(complete_code_bytes=321,routines=groups,findings=[
 '10365ba0 rejects nonstringtype5, initializes defaultinvalidsymbol, rereads type/data, unchangedraw32symbol exitsfalse, changed invokes writegate103623b0.',
 '10366100 onallowedchange optionally createsrecord10365770/model79 BEFORE writing suppliedraw32symbol intoattribute4.',
 'Flag100 plusowner1035dc00 permitsowner vslot0 notification. Rereads attributetype1/15 referenceboolean then1035eff0 translates topropagatemask3(reference)or2(other).',
 'Modelc180 OR1000 forreference or2000 forother. Type5normalpath propagates2 andOR2000, notstructuralmask3.',
],limitations='Writegate103623b0/change-record helper/notification semanticnames/runtimeintegration pending; nonstringbranch cannot normally reachtype1/15 withoutmutation.')
Path('outputs/string-attribute-update-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified string update/compare/propagation:',321,'bytes')
