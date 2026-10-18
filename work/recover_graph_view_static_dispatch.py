import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10074110,0x10074172)]
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
table=[im.word(0x10074174+i*4) for i in range(4)]
assert table==[0x10074128,0x1007413e,0x1007414c,0x1007415f]
result=dict(complete_code_bytes=98,routines=groups,jump_table=[hex(v) for v in table],findings=['Kind0 only; unsigned ID0..3 only.', 'ID0 forwards argv1 pointee address as QPoint reference to virtual slot50; ID1 tailcalls virtual slot51.', 'IDs2 and3 both dereference argv1 to QKeyEvent pointer and call100d3ef0.'],limitations='Virtual implementations and key-event handler semantics not yet recovered; argv array and pointers widened natively.')
Path('outputs/graph-view-static-dispatch-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified graph view static dispatcher:',98,'code bytes and16 table bytes')
