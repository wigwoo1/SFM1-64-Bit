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
    if va>=0x10361d26:break
    if va<0x10361c40:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x10361c40
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x10361d26
Path('outputs/element-notification-constructor-recovery.json').write_text(json.dumps(dict(complete_bytes=230,instructions=spans,findings=[
 '10361c40 clears state3e low5 bits (AND e0) and packed40 low4 bits (AND fffffff0). Thus required aggregate bits2..3 start0, not3.',
 'Sets notification list heads44/48/4c/50/54/58 null. Preserves higher state/packed bits rather than overwriting entire fields.',
 'Full constructor also initializes handles/attribute fields, interns symbols, sets owner38 from fifth argument, registers owner, creates name attribute and copies UUID. Notification-only candidate does not implement these services.',
 'The prior combined cycle explicitly assumed initial aggregate3; it was a conditional fixture, not native initialization evidence. Updated cycle uses constructor-proven zero initialization.'
],limitations='Full constructor bytes verified, but helper side effects after initialization may still alter fields. Notification-only candidate is not full constructor; actual runtime parity and editor integration pending.'),indent=2)+'\n')
print('Verified constructor bytes:',230)
