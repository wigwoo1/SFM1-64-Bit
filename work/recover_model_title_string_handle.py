import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10356360,0x103563c2)]
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
assert im.word(0x10b37094+97*4)==0x10356360
result=dict(complete_code_bytes=98,routines=groups,findings=[
 'Model97 validates32bit handle: sentinel,low20 index bound,generation11 match against full low31 slotword,and slot highbit clear; slot payload pointer must be nonnull.',
 'Returns payload first interned string pointer, mappingffffffff to installed emptystring; invalid handle returns null, distinct from valid empty.',
 'Pool is modelc1a0/countc1ac; semantic filename identity still needs payload initialization evidence. Native slot payload and string must be pointer width.'
],limitations='Pool registration and string identity/production integration pending.')
Path('outputs/model-title-string-handle-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified model97 string handle lookup:',98,'bytes')
