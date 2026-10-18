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
    if va>=0x10356f30:break
    if va<0x10356ed0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x10356ed0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x10356f30
Path('outputs/element-owner-release-recovery.json').write_text(json.dumps(dict(complete_bytes=96,instructions=spans,findings=[
 'Slot=handlelow20; unsigned bounds check only. No sentinel or supplied generation validation.',
 'Increments metadata low31 preserving old highbit. If previously active, sets inactive highbit and decrements livecount; alreadyinactive skips decrement but still increments generation.',
 'Clears object pointer. Requeues slot via10354c10 only if new low31 generation<2047; at2047 orabove slot retired from reuse.',
 'Repeated release of inactive slots below limit can enqueue again; callers must enforce lifecycle, do not add an unproven idempotence guard.'
],limitations='Complete bytes verified; ring enqueue helper and callers/synchronization/original execution unverified. Pure candidate requires adapter enqueue contract.'),indent=2)+'\n')
print('Verified owner release bytes:',96)
