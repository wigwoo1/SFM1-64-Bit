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
    if va>=0x1029ac1e:break
    if va<0x1029aae0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1029aae0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1029ac1e
Path('outputs/editor-map-fallback-recovery.json').write_text(json.dumps(dict(complete_bytes=318,instructions=spans,findings=[
 '1029aae0 primary resolve model18 guard then outer save61/disable60(0), primary secondresolve, currenttime1029aa20, inner save61/disable60(0).',
 'Nonnull primary vslot23 supplies track; maps time via104bc680(primary,time,0), resolves matching clip via104d3760(track,mapped).',
 'Candidate vslot4 checks globaltype token10eb1124 before restoring inner scope10007370, then checks again after innerrestore; secondfailure clearsresult.',
 'All paths after scopes restore inner then outer; missingfirstprimary returnsnull withoutscope. Helpers/type token/class interpretation remain explicit obligations.'
],limitations='Complete bytes verified; currenttime/track lookup/clip selection/type registration/scope60-61 semantics and native editor integration pending.'),indent=2)+'\n')
print('Verified map fallback bytes:',318)
