import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x102a34c0,0x102a3542),(0x10975a20,0x10975ad1)]
groups=[[],[]];active=None
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
Path('outputs/editor-map-name-recovery.json').write_text(json.dumps(dict(complete_bytes=307,routines=groups,findings=[
 '102a34c0 first extracts engine current map via10975a20 into256-byte local. Prefer nonempty supplied name, then nonempty102a2210 project name, then engine local; final extraction into caller buffer/capacity.',
 '10975a20 extracts base name: last dot after last slash/backslash strips extension; removes directory prefix. No lowercasing or drive-colon removal. Bounded copy via10a16550 with computed count then final nul.',
 'Null/empty source writes destination0 unconditionally; original zero-capacity edge not silently fixed. Native candidate supports positive capacity and normal nonoverflow lengths only.',
 'Corrects prior map protocol copy-only assumption: map command helper uses basename extraction, so maps/stage.bsp becomes stage.'
],limitations='Complete bytes verified; copyhelper10a16550 exact semantics/original execution parity/current project mapgetter helpers/native UI integration pending.'),indent=2)+'\n')
print('Verified map selection/base-name bytes:',307)
