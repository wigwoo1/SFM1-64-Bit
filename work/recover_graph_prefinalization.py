import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100c09b0,0x100c0a97)]
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
result=dict(complete_code_bytes=231,routines=groups,findings=['Begin imported scope; snapshot optional ownerfield14 and signedcount24, live vector18 per record; release nestedfield18 in each140-byte x86 record via104fd6d0; clear ownerfield14.', 'Get102cca90 currentgraphstate, no null guard; snapshot count84, live handlearray78 eachiteration; resolve/filter using symbol10effc4c; filtered elements call10741340.', 'Reload child344, optional10275f50; end imported scope.'],limitations='Nested record release, element refresh and child update semantics pending; native record stride supplied by adapter.')
Path('outputs/graph-prefinalization-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified graph prefinalization:',231,'bytes')
