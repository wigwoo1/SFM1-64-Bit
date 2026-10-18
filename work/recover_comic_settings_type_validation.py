import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1000cb10,0x1000cb37)]
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
vtable=0x10a8b0c0
assert im.word(vtable+16)==0x1000cb10
locator=im.word(vtable-4);descriptor=im.word(locator+12)
name=im.string(descriptor+8);assert name=='.?AVCDmeComicSettings@@'
result=dict(complete_code_bytes=39,routines=groups,vtable=hex(vtable),rtti=name,findings=[
 'CDmeComicSettings vslot4 accepts own typeglobal10e7786c, base CDmElement global10e9a684 or wildcardffffffff.',
 'At least this concrete derived implementation preserves base compatibility; this does not prove every serialized class inherits or dispatches identically.'
],limitations='Runtime type registration/global values and other derived classes/native session adapter validation pending.')
Path('outputs/comic-settings-type-validation-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified derived type:',name,'39bytes')
