import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10390d50,0x10390e4d)]
groups=[[] for _ in ranges];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16);active=None
    if va>=ranges[-1][1]:break
    for i,(start,end) in enumerate(ranges):
        if start<=va<end:
            active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());groups[i].append(active);break
for (start,end),spans in zip(ranges,groups):
    cursor=start
    for s in spans:
        va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
        assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
        cursor+=len(raw)
    assert cursor==end
result=dict(complete_code_bytes=253,routines=groups,findings=[
 'Begins model slot121(label,1,256), logs, saves flag74 bit3 and disables it.',
 'Uses initial undo count minus1 as loop bound, but pops live undo tail each iteration before record slot0 replay. Appends to redo before reading replayed record marker slot6; any nonzero marker stops group.',
 'Restores only recording bit3 in live flags after end log. If signed field54<=0 clears78/7c. Ends model slot122(0), including empty stack.'
],limitations='Native adapters for notifications, logging, vector growth and replay remain external; runtime integration pending.')
Path('outputs/undo-operation-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo operation:',253,'bytes')
