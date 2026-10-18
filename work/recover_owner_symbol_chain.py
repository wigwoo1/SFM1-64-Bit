import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x103543f0,0x1035443d),(0x10975120,0x10975157)]
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=ranges[-1][1]:break
    if not any(a<=va<b for a,b in ranges):active=None;continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
for a,b in ranges:
    cursor=a
    for span in spans:
        va=int(span['va'],16)
        if not a<=va<b:continue
        raw=bytes.fromhex(span['bytes']);assert cursor==va
        assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
        cursor+=len(raw)
    assert cursor==b
report=dict(verified_complete_instructions=spans,complete_bytes=sum(b-a for a,b in ranges),findings=[
 '103543f0 is linked-chain search: arg2 head, arg3 exclusive stopping node; follows node4. No allocation/insertion.',
 'Full32-bit hash at node0 key0 must equal input key0 before10975120 compares key strings4.',
 '10975120 compares unsigned bytes case sensitively, returns0 if equal, -1 if first mismatch byte smaller else1.',
 'First matching node before stop returned; empty range returnsnull. Nonnull reachable nodes until stop original precondition.',
 'Locked retry searches only chain range before bucket4 stop, not a different comparison mode.'
],limitations='Complete77-byte chain helper and55-byte string comparator verified. Hash algorithm, concurrent publication/stop-node lifetime and full symbol manager original execution remain unverified.')
Path('outputs/owner-symbol-chain-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified complete symbol chain/comparator bytes:',report['complete_bytes'])
