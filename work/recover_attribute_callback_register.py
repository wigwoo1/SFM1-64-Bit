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
    if va>=0x1035df3d:break
    if va<0x1035def0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035def0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035df3d
Path('outputs/attribute-callback-register-recovery.json').write_text(json.dumps(dict(complete_bytes=77,instructions=spans,findings=[
 '1035def0 nullattribute returns without allocation/flag change. Nonnullattribute allocates12-byte node via10f49a58/1097aa70.',
 'Allocated node callback0=suppliedcallback, value4=attributepointer,next8=0; then prepends retained element48. No duplicate or nullcallback rejection.',
 'Always ORs attributeflags10 with2000 for nonnullattribute, even if allocation returnednull. Nullallocation leaves list unchanged.'
],limitations='Complete installed bytes verified; nodeallocator failure/factory wiring/originalexecution/nativeUI integration pending. Native node layout supplied separately, not12-byte x86 assumption.'),indent=2)+'\n')
print('Verified attribute callback register bytes:',77)
