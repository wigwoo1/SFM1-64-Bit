import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x107358e0,0x10735b76)]
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
result=dict(complete_code_bytes=662,routines=groups,findings=['Find graphCurve reference through100bbf60 then104cd3e0, null gates; snapshot virtualslot62 count; percomponent live handle5c resolution and slot36 type.', 'Type10 components selected by rotating mask against field70; other types component0only.', 'Compare source count84+component24 against105079b0; then each mapped rawtime through indexarray78+component24 and timearray258+component24 with10517210. Equal arrays skip setter.', 'Changed arrays assembled in source order then10517340(component,vector), free temporary storage; rotate mask1 bit eachcomponent.'],limitations='Full semantic/native implementation pending; initial equality gate reconstructed separately.')
Path('outputs/graph-bookmark-update-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified bookmark update:',662,'bytes')
