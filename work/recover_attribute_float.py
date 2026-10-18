import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100276d0,0x1002772c),(0x103624c0,0x103624e4),(0x10362780,0x10362802)]
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
table=[im.word(0x10362804+4*i) for i in range(6)]
assert table==[0x103627c2,0x103627d7,0x10362799,0x103627fe,0x103627fe,0x103627e5]
constants={hex(v):hex(im.word(v)) for v in [0x10a8a56c,0x10a8dfd4]}
result=dict(complete_code_bytes=258,routines=groups,table=table,constants=constants,findings=[
 '100276d0 direct type3 returns data pointer; other types check103624c0 accepting types2/3/4/7. Unsupported returns staticfloat zero, not caller missingattribute24 fallback.',
 '10362780 type2 signed32 SSE conversion;3 x87floatcopy;4 booleanbyte->0/1;7 signed32 SSEfloat multiplied by installedconstant10a8dfd4. Shared scratch output.',
],limitations='Complete code plus24tablebytes verified; x87specialfloat/controlstate/runtime sharedstorage concurrency and native integration pending.')
Path('outputs/attribute-float-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified float accessor bytes/table/constants:',258,24,constants)
