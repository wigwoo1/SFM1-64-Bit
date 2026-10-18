import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1038f160,0x1038f20e),(0x1038f320,0x1038f35e)]
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
Path('outputs/element-framework-reset-recovery.json').write_text(json.dumps(dict(complete_bytes=236,routines=groups,findings=[
 'Slot10 reset1038f320: if phaseb4==0 sets1/calls1035c820, sets2/calls1038f210; always setsphase0 afterward. Nonzero phase skips both helpers.',
 '1038f160 first captures vector8c count98, resolves each live handle and calls103603f0, then clears count98.',
 'Then captures callbackvectora0 countac, invokes each slot0; callback listcount not cleared.',
 'Finally captures vector78 count84, resolves each live handle/calls1035dbd0 stateclear, then clearscount84. Counts captured separately after preceding effects.'
],limitations='Complete bytes verified; helper103603f0/1038f210 effects, interface registration and actual runtime phase invocation timing/original execution remain unresolved.'),indent=2)+'\n')
print('Verified framework reset/cleanup bytes:',236)
