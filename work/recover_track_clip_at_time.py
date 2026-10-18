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
    if va>=0x104d3802:break
    if va<0x104d3760:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x104d3760
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x104d3802
Path('outputs/track-clip-at-time-recovery.json').write_text(json.dumps(dict(complete_bytes=162,instructions=spans,findings=[
 '104d3760 requires track98==3 and count68!=0 before calling104d34e0 preparation; captures signedcount afterward.',
 'Ascending live handles vector5c with raw4-byte handles, modelslot18 resolve, candidate typevslot4 token10eb10a4; skips invalid/ineligible.',
 '104bba30 start <= signedinttime and104bba80 end > signedinttime. First matching interval [start,end) returned; no inclusive end or overlap tie-break beyond vector order.',
 'No seconds/float interpretation introduced. Preparation may change vector/count; count snapshot after helper, table pointer read every iteration.'
],limitations='Complete bytes verified; preparation/start/end/type symbol helper effects and actual native session/editor integration pending.'),indent=2)+'\n')
print('Verified track clip selection bytes:',162)
