import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10400d50,0x10400d56),(0x104027f0,0x10402809),(0x10402c90,0x10402cb1),(0x1040f040,0x1040f04f)]
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
result=dict(complete_code_bytes=79,routines=groups,findings=[
 '10402c90: global10f4c840 nonzero calls1040a7b0(address10f4c844) then zeros initialization word. Zero skips.',
 '104027f0: global10f4c83c nonzero calls1040f040 then zeros initialization word. Zero skips.',
 '1040f040 calls10431100,10430be0 then tails104304d0 in order.',
 '10400d50 import tail10a7e1c4 resolves WS2_32.dll ordinal116 in installed import table; exported semantic name not established by this receipt.'
],limitations='Inner disposal routines and ordinal export name pending; no actual networking/library cleanup executed.')
Path('outputs/controller-subsystem-cleanup-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified subsystem cleanup guards/chain:',79,'bytes')
