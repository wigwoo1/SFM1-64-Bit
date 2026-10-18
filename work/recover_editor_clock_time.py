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
    if va>=0x10275693:break
    if va<0x10275640:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x10275640
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x10275693
Path('outputs/editor-clock-time-recovery.json').write_text(json.dumps(dict(complete_bytes=83,instructions=spans,initial_cache_state='No file-backed bytes at 10f321d0; virtual initialization and runtime writes not established.',findings=[
 '10275640 engine10fd4398 slot47 returns x87 sample. Stores roundedfloat local but compares unpopped sample with float global10f321d0; unordered or <= skips update.',
 'When sample greater, stores its roundedfloat into sharedcache. Always converts cachedfloat through1097b170 thenwrites calleroutput.',
 'Direct disassembly references to10f321d0 found only in this routine; no reset reference established. Does not imply runtimecache can never reset via alias.',
 'Nativecandidate preserves caller-owned sharedfloat cache; engine return precision/runtime lifetime still explicit obligations.'
],limitations='Complete bytes verified; engine47 identity/x87 precision/source time/runtime cache reset/original execution/native editor integration pending.'),indent=2)+'\n')
print('Verified editor clock bytes:',83)
