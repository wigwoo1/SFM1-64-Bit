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
    if va>=0x10354cf7:break
    if va<0x10354c10:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x10354c10
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x10354cf7
Path('outputs/element-owner-recycle-queue-recovery.json').write_text(json.dumps(dict(complete_bytes=231,instructions=spans,findings=[
 'Queue pointer0/capacity4/growsize8/headc/tail10. Empty tailFFFFFFFF ensures minimumcapacity1 and initializes both indices0.',
 'Nonempty advances tail with wrap; if next==head grows through102bdbd0 delta1, recomputes tail relative to newcapacity.',
 'If head!=0 after growth, memmoves oldcapacity-head uint32 slots right by capacitydelta and advances head. Preserves wrapped queue order.',
 'Writes supplied uint32 slot at final tail. Queue elements stay32-bit indices in x64; data pointer widens separately.'
],limitations='Complete byte verification; allocator/external storage/failure/invalid ring invariants/original execution pending. Candidate covers normal valid ring only.'),indent=2)+'\n')
print('Verified recycle queue bytes:',231)
