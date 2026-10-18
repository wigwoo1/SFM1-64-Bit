import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100a4830,0x100a523d)]
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
vt=0x10aa9978;col=im.word(vt-4)
assert im.string(im.word(col+12)+8)=='.?AVCQSFMController@@' and im.word(col+4)==0x18 and im.word(vt)==0x1009cbd0
result=dict(complete_code_bytes=2573,routines=groups,findings=[
 'Complete constructor100a4830..100a523d installs CQSFMController embedded undo-listener vtable10aa9978 at owner+18; same function registers owner+18 via known model slot124 at100a51f7.',
 'This closes constructor-to-registration-to-callback provenance. Callback fields relative listener219/228 correspond owner231/240; native layout must use actual members.',
 'Constructor initializes owner238 and240 timestamps after registration. Other constructor dependencies remain imported or unrecovered.'
],limitations='Complete bytes verified, not complete constructor semantic implementation; Qt object lifecycle/native UI integration pending.')
Path('outputs/controller-listener-constructor-provenance-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified full controller constructor provenance:',2573,'bytes')
