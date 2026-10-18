import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10391130,0x1039118e)]
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
result=dict(complete_code_bytes=94,routines=groups,findings=[
 'Disabled recording returns false. Snapshots signed positive counter70 BEFORE cancellation log and group-end call.',
 'Invokes10391080 ignoring return; checks live depth54 afterward. Only zero depth plus saved positive counter invokes undo10390d50 then redo cleanup10390bd0 and returns true.',
 'Cancellation inherits end-group context merging; isolation to only newly submitted records is not proven and must not be imposed on native candidate.'
],limitations='Production runtime binding and cancellation behavior with merged contexts require further validation.')
Path('outputs/undo-group-cancel-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo group cancel:',94,'bytes')
