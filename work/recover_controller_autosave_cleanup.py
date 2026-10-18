import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1009cce0,0x1009ccfe),(0x100a25e0,0x100a26c8)]
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
strings={hex(v):im.string(v) for v in (0x10aa97d8,0x10aa97d0)}
result=dict(complete_code_bytes=262,routines=groups,strings=strings,findings=[
 'Cleanup builds260-byte path via101b3190(.sfm_autosave.loc) then format%s%s. Formatting wrapper1009cce0 passes varargs to10975800 with capacity260.',
 'Filesystem service subinterface+4 slot14(path,null,buffer260,260,0,0) returns success byte. On success calls main filesystem slot18(path,null), extracts260-byte buffer via1097e660, calls same slot18(secondpath,null).',
 'Always destroys owned temporary buffer and checks security cookie; failure skips both slot18 calls. File operation identities not yet proven.'
],limitations='Path service/filesystem vtable identities and buffer extraction semantics pending; original files were only read, no cleanup executed.')
Path('outputs/controller-autosave-cleanup-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified autosave cleanup/helper:',262,'bytes')
