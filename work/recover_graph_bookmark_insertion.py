import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1017ee80,0x1017ef0b)]
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
result=dict(complete_code_bytes=139,routines=groups,findings=['Amount0 returns start unchanged. Otherwise wrappedcount+amount signed>capacity calls102bdbd0(wrappedoldcount-capacity+amount).', 'Adds amount to LIVEcount, snapshotsbase, publishescachedbase10; trailing=wrappednewcount-start-amount. Positive trailing/amount calls10974fe0(dest start+amount,source start,bytes trailing*4).', 'Positiveamount initializes each inserted rawtime80000000 with LIVEbase reload and computedaddress nonnull guard; returnsstart.'],limitations='Growth/copy helper delegated; invalid ranges not guarded original.')
Path('outputs/graph-bookmark-insertion-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified bookmark insertion:',139,'bytes')
