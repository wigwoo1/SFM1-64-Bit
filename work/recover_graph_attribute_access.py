import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10002780,0x100027e0),(0x1035dd50,0x1035dd91)]
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
result=dict(complete_code_bytes=161,routines=groups,findings=['Lookup interns name via modelslot50, then reads live elementhead30, traverses node next0 comparing interned symbol14; firstmatch/null.', 'Accessor initializes separate default handle structure once; attribute low5 type bits==1 returns data4 directly.', 'Other types call10362e50; lowbyte true calls103630a0 into sharedscratch10f296fc and returns scratch irrespective conversion result; false returns default10f29700.'],limitations='Conversion callbacks pending; symbol fields native pointer width, element handles remain32bit.')
Path('outputs/graph-attribute-access-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified attribute access:',161,'bytes')
