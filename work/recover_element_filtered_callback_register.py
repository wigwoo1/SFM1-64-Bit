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
    if va>=0x1035dfcc:break
    if va<0x1035df40:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035df40
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035dfcc
Path('outputs/element-filtered-callback-register-recovery.json').write_text(json.dumps(dict(complete_bytes=140,instructions=spans,findings=[
 '1035df40 allocates12 via10f49a28/1097aa70; callback0, full supplied mask4, next8; prepend element44. No duplicate or nullcallback rejection.',
 'After allocation (including failure), capture packed element40; prior=(packed>>2)&3; narrowed=prior&suppliedMask.',
 'Queue handle1c through1038f5e0 if state3e bit10 set, active low2 includes narrowed, but active low2 does not include all prior bits.',
 'After optional queue call reread element40 and replace only bits2..3 with narrowed. Unrelated bits mutated by queue remain. Mask meanings beyond recovered bit operations are not assumed.'
],limitations='Installed bytes verified; allocator and queue global binding, original execution parity, production ABI and native editor integration pending.'),indent=2)+'\n')
print('Verified filtered callback registration bytes:',140)
