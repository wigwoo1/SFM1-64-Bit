import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x104311e0,0x1043126f)]
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
result=dict(complete_code_bytes=143,routines=groups,source_string=im.string(0x10b5e5ac),findings=[
 '104311e0 ensures globalservice10f4cb2c: if null invokes10442510(9,1,source,215), reloads pointer, installs defaulttable10b5e580 only if stillnull, then10442510(10,1,source,218).',
 'Input code transformed by right/left shifts to code&fffff000. Local key passed by pointer to live service callback slot2(offset8). Null result returns0; nonnull returns recordfield1(offset4).',
 'Native service pointers, callbacks and returned payload field need pointer-width treatment; code remains32. Exact guard10442510 and default callback implementation remain pending.'
],limitations='Native adapter fixture only; returned field semantic identity and original execution parity not proven.')
Path('outputs/controller-registration-lookup-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified registration lookup:',143,'bytes')
