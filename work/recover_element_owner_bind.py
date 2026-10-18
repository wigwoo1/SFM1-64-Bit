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
    if va>=0x10354bbb:break
    if va<0x10354b70:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x10354b70
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x10354bbb
Path('outputs/element-owner-bind-recovery.json').write_text(json.dumps(dict(complete_bytes=75,instructions=spans,findings=[
 'RejectFFFFFFFF handle, unsigned slot>=count, or metadata low31 not equal extracted11-bit generation. Handle highbit ignored.',
 'Matching entry with metadata highbit set increments pool livecount and clears metadata highbit before pointer assignment. Unlike resolve, binding permits reactivation.',
 'Writes supplied object pointer even null or replacing existing; does not modify owner element counter or generation.'
],limitations='Complete bytes verified. Reactivation callers/free-ring lifecycle/production pool synchronization/original execution and imported owner binding unverified.'),indent=2)+'\n')
print('Verified owner binding bytes:',75)
