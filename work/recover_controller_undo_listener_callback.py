import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1009cbd0,0x1009cc0c)]
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
vt=0x10aa9978;col=im.word(vt-4);name=im.string(im.word(col+12)+8)
assert name=='.?AVCQSFMController@@' and im.word(col+4)==0x18 and im.word(vt)==0x1009cbd0
result=dict(complete_code_bytes=60,routines=groups,rtti=name,interface_offset=24,findings=[
 'Embedded post-action interface vtable10aa9978 has CQSFMController RTTI and COL offset18; slot0 is1009cbd0.',
 'Callback reads global10f32568 child344; if present calls10275f20(child,1). Writes byte1 at listener-relative219 then RDTSC64 at listener-relative228.',
 'Offsets are relative to embedded interface this; native member identities and child helper semantics remain unresolved.'
],limitations='Constructor connection to concrete registration site, field names and production UI behavior still require verification.')
Path('outputs/controller-undo-listener-callback-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified CQSFMController undo listener callback:',60,'bytes')
