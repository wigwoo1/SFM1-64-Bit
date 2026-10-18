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
    if va>=0x1038f45a:break
    if va<0x1038f360:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1038f360
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1038f45a
Path('outputs/element-state-queue-consume-recovery.json').write_text(json.dumps(dict(complete_bytes=250,instructions=spans,findings=[
 'Managerphaseb4==0 runs phase1 CDataModel1035c820 then phase2 1038f210. Always phase3 helper103999f0, phase4 ascending callback list50/count5c slot2.',
 'Captures signed queuecount70 after phase4 callbacks, setsphase5; ascending current table64 reads each handle, resolves service slot18; resolved element vslot3 then1035dbb0 resets attribute/state marks.',
 'Captured count remains fixed during callbacks, current table reread each iteration. Appended handles beyond snapshot are not processed, and final count70 reset0 discards them in this pass.',
 'After reset calls1038f160 then CDataModel10356e60 symbol primary publication; ends phase6. Does not restorephase0 here.'
],limitations='Complete bytes verified; earlier phases/global service identities/callback effects/1038f160 queue clearing contracts/original execution remain pending. Candidate limited to phase5 scan/reset.'),indent=2)+'\n')
print('Verified queue consume bytes:',250)
