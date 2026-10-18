import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1035db90,0x1035dbae),(0x1035dbb0,0x1035dbd0),(0x1035dbd0,0x1035dbd9),(0x1035dbe0,0x1035dbe5),(0x1035dbf0,0x1035dbf5),(0x1035dc00,0x1035dc09),(0x1035dc10,0x1035dc14)]
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
total=sum(end-start for start,end in ranges)
Path('outputs/element-state-flags-recovery.json').write_text(json.dumps(dict(complete_bytes=total,routines=groups,findings=[
 '1035db90 tests elementbyte3e bit1. If clear queues elementhandle1c via1038f520(global10e9ccf4), then ORs byte3e with3. No repeated queue while bit1 set.',
 '1035dbb0 captures attributehead30 before clearing element3e bit1, walks attrnext0 and clears only flagsword10 bit80.',
 '1035dbd0 clears element3e bit10 and element40 low2bits; dbe0 clears3e bit4; dbf0 setsbit4; dc00 queriesbit4.',
 '1035dc10 returns element+1c. Earlier cleanup decision recordc is therefore element28 and record4 is element20, not a separately allocated record.'
],limitations='Complete installed bytes; state names/queue1038f520 effects/packed element20 fields/original execution/UI integration remain pending.'),indent=2)+'\n')
print('Verified element state/accessor bytes:',total)
