"""Recover owner transition dispatch; phase3 direct path only translated."""
import json, re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start,end=0x10361eb0,0x10361f30
cursor=start
instructions=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m: continue
    va=int(m[1],16)
    if va>=end: break
    if va<start: continue
    raw=bytes.fromhex(m[2]);assert va==cursor
    assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
    instructions.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
assert cursor==end
report=dict(verified_complete_instructions=instructions,complete_span_bytes=end-start,
 findings=[
 '10361eb0 dispatches on second argument equality3. Phase3 jumps directly to10361eff.',
 'Direct path: release(old owner38, element1c), store first argument into owner38, register(new owner38, element1c). No equality shortcut or new-handle validation before assignment.',
 'Other phases construct44-byte local via10034940, reserve1000 via10034a40, pass new/old owner and phase plus third argument to10361a40, destroy local100349c0.',
 'First argument is stored owner handle; third argument is unused in phase3.',
 'Destructor1035ee80 also releases current owner before later cleanup; full destructor not yet translated.'
 ],limitations='Complete128-byte routine verified through10361f30 exclusive boundary. Only phase3 direct path semantics translated. Other phase undo/reference traversal, manager type/file identity and original runtime parity pending.')
Path('outputs/element-owner-transition-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified owner transition complete span:',end-start)
