import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x101b3190,0x101b31e2),(0x10975cf0,0x10975d16)]
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
result=dict(complete_code_bytes=120,routines=groups,search_path_id=im.string(0x10aca70c),findings=[
 '101b3190 ignores supplied caller arguments, returns shared global buffer10f2d150. First byte nonzero skips filesystem call and normalization.',
 'On empty cache calls main filesystem slot16(search-path ID,null,cache,260), truncates at first semicolon via10a12c70, normalizes slash/backslash using10975cf0(cache,92). Empty result retries next call.',
 '10975cf0 walks to NUL and replaces both slash byte47 and backslash byte92 with supplied low byte; no case conversion or duplicate removal.',
 'Cleanup100a25e0 preserves earlier pushed suffix as second format argument after prefix, format%s%s. Native cleanup adapter must explicitly receive suffix.'
],limitations='Filesystem slot16 and character search helper identities remain pending; semantic adapter, not production filesystem binding.')
Path('outputs/controller-path-prefix-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified cached path prefix and slash normalization:',120,'bytes; path ID:',result['search_path_id'])
