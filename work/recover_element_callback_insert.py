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
    if va>=0x1035fea7:break
    if va<0x1035fe30:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035fe30
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035fea7
Path('outputs/element-callback-insert-recovery.json').write_text(json.dumps(dict(complete_bytes=119,instructions=spans,findings=[
 'Inline-pointer vector pointer0/capacity4/growsize8/inlinecapacityc/count50/cache54. Insertion indexarg1, sourceaddressarg2 dereferenced after shift.',
 'If capacity insufficient and growsize negative, detaches via10a61fb0(inlinecapacityc) before growth102bdbd0(delta). External inline storage must not be reallocated/freed as owned heap.',
 'Count increment/cache update then shifts trailing4-byte pointer entries, stores sourcefirstword, returns index. No duplicate check or filtering.'
],limitations='Complete bytes verified; detachhelper/original execution/aliasing/failure and full collection semantics pending. x64 pointer entries require native sizeof(pointer), inline vector layout cannot reuse offsets.'),indent=2)+'\n')
print('Verified callback pointer insert bytes:',119)
