import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10391080,0x10391128)]
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
result=dict(complete_code_bytes=168,routines=groups,findings=[
 'Recording disabled returns false. Logs then decrements live depth54; nonzero result returns false without counter reset.',
 'At zero depth, signed positive counter70 yields true and clears redo. Equal nonnull contexts78/7c scans undo backwards, skips nulls and clears newest nonzero marker, merging groups.',
 'Zero depth always resets counter70. Native label/context values preserve pointer width.'
],limitations='Runtime integration and original execution parity pending.')
Path('outputs/undo-group-end-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo group end:',168,'bytes')
