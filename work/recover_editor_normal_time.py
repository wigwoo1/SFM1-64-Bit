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
    if va>=0x1029aa1d:break
    if va<0x1029a950:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1029a950
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1029aa1d
Path('outputs/editor-normal-time-recovery.json').write_text(json.dumps(dict(complete_bytes=205,instructions=spans,findings=[
 '1029a950 engine10fd4398 slot29 gate. Iftrue and field47c nonzero OR one480/484/488/48c==2: time430 sentinel80000000 returns stored684; otherwise clock10275640 minuscaptured430 modulo32bits.',
 'Else capturesstored684; only if69c/228/2a8 allzero and6c0 nonzero calls10298ad0(stored,0), otherwise storedreturned.',
 'Only four mode fields tested, unlike102b6180 operation guard that also tests490. Keep these gates distinct.',
 'Nativecandidate names rawfields until semantic identities verified; external clock/adjustment helpers remain adapters.'
],limitations='Complete bytes verified; engine state meaning/clock/adjustment helper effects/servicebinding/original execution/native editor integration pending.'),indent=2)+'\n')
print('Verified normal time bytes:',205)
