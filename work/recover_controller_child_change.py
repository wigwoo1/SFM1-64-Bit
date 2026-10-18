import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x102bc460,0x102bc516)]
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
strings={hex(v):im.string(v) for v in (0x10ae7604,0x10ae756c)}
result=dict(complete_code_bytes=182,routines=groups,strings=strings,findings=[
 'Absent child uses installed default title string. Present child calls102787d0 then formats512-byte buffer if input lowbyte nonzero, otherwise passes child string directly; title setter1098bf80 targets controller33c.',
 'Nonzero lowbyte and controller5a5 plus nonnull100013f0 result invoke that object slot9; then optional100a6250 result receives live child344 and full input via1009d0e0.',
 'Title formatting, helper object identities and native runtime binding remain unresolved; lowbyte controls branch while full input is forwarded.'
],limitations='Complete static code verified; helper internals and UI rendering parity pending.')
Path('outputs/controller-child-change-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified controller child-change routine:',182,'bytes',strings)
