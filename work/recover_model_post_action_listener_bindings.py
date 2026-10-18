import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10353b30,0x10353b3f),(0x10353b40,0x10353b4f),(0x10353b50,0x10353b5b)]
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
bindings={124:0x10353b30,125:0x10353b40,126:0x10353b50}
for slot,va in bindings.items():assert im.word(0x10b37094+slot*4)==va
result=dict(complete_code_bytes=41,routines=groups,bindings={str(k):hex(v) for k,v in bindings.items()},findings=[
 'CDataModel slots124/125/126 respectively register/remove/dispatch post-action listeners. All wrappers forward original this+c118 to nested stack; register/remove forward caller pointer unchanged.',
 'Native build must bind corresponding native member rather than apply original x86 c118 byte offset.'
],limitations='Concrete editor listener implementations and production runtime binding pending.')
Path('outputs/model-post-action-listener-bindings-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified model post-action bindings:',41,'bytes; slots124-126')
