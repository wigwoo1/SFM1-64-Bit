import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10431320,0x10431359),(0x1043cfa0,0x1043cfad)]
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
result=dict(complete_code_bytes=70,routines=groups,findings=[
 '10431320 is HASH callback, not comparison: folded=code^((((code>>12)^code)>>12)&fff); return folded^((folded%19)*13) using multiply-high division sequence.',
 '1043cfa0 is COMPARISON callback, returns wrapped32(firstcode-secondcode), not signed ordering ternary.',
 'Corrects earlier container receipt/test parameter names:10442c60 first argument hash10431320, second comparison1043cfa0. Argument address order was previously correct; semantic labels reversed.'
],limitations='Native arithmetic fixture and static bytes; container implementation/production integration pending.')
Path('outputs/controller-registration-hash-comparison-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified registration hash/comparison:',70,'bytes')
