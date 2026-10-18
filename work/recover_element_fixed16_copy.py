import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1097bc10,0x1097bc2d)]
groups=[[]];active=None
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
result=dict(complete_code_bytes=29,routines=groups,findings=[
 '1097bc10 cdecl(src,dst) copies first8bytes before loading/copying second8; unaligned MOVQ, no null/overlap check.',
 '10361c40 passesconstructorarg2 source pointer and element+4 destination, preserving16byte field beforeoriginal14 name field.',
 'Factoryarg4 thus provides16byte source. Identity/UUID meaning inferred from existing DMX16byteIDs but not yet proved by native naming.'
],limitations='Byte verified; exact field semantic naming/original execution parity/native object reconstruction pending.')
Path('outputs/element-fixed16-copy-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified sequential fixed16 copy:',29,'bytes')
