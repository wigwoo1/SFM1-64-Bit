import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x102a2210,0x102a2269),(0x104bbf60,0x104bbf71)]
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
assert im.string(0x10a8a4f0)==''
Path('outputs/editor-project-map-getter-recovery.json').write_text(json.dumps(dict(complete_bytes=106,routines=groups,findings=[
 '102a2210 primary handle this84 resolves modelslot18 twice (first guard, second for104bbf60). If first missing returns staticempty without secondary fallback.',
 'Primary104bbf60 returns nonempty string ifavailable, else1029aae0 obtains fallback object; ifnonnull tailcalls same getter, else staticempty.',
 '104bbf60 reads objectb0 pointer; ffffffff maps to staticempty10a8a4f0, other values includingzero returned unchanged. Native x64 sentinel representation must be chosen explicitly by native string storage adapter.',
 'Selection102a34c0 performs engine extraction first, then supplied/project/engine priority. Null returned from fallback getter is allowed and handled by selection.'
],limitations='Complete bytes verified; fallback-object helper1029aae0/class identity/lifetime of second primary resolve/native string storage representation/runtime integration pending.'),indent=2)+'\n')
print('Verified project map getter bytes:',106)
