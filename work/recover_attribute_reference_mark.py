import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=0x10362421:break
    if va<0x103623b0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x103623b0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x10362421
Path('outputs/attribute-reference-mark-recovery.json').write_text(json.dumps(dict(complete_bytes=113,instructions=spans,findings=[
 'Attributeflags10 bit20 rejects before service query. Service10e9ccf8 slot8 permits0/1/2, or4 when flags400 clear; other values returnfalse.',
 'If flags80 clear and2000 set, invokes owner8 helper1035e3a0(attribute) before flags update.',
 'Reads current owner8 again, ORs flagsword10 with4080, calls1035db90(owner), returnstrue.',
 'No reference value removal in this routine: callback marks flags and owner state. Do not infer nulling scalar references or erasing arrays.'
],limitations='Complete bytes verified; flag names/helper effects/service phase ABI/original execution and production editor binding unresolved.'),indent=2)+'\n')
print('Verified attribute reference mark bytes:',113)
