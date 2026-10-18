import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10430c50,0x10430ce9)]
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
table=[im.word(0x10b5e580+i*4) for i in range(11)]
assert table[2]==0x10430c50
result=dict(complete_code_bytes=153,routines=groups,default_service_table=[hex(v) for v in table],findings=[
 'Default service table10b5e580 slot2=10430c50. Same guarded default-install logic as104311e0.',
 'Reloads live service slot0 and calls with0 to get container. Null returnsnull without lookupguards. Nonnull10442510(5,1,source,298),10442e30(container,key),10442510(6,1,source,300).',
 'Saves lookupresult before closing guard then returns saved record. Table pointers/callbacks native-width in candidate; code key remains32.'
],limitations='Default container slot010431510, lookup10442e30 and guard helper internals pending. Table11 entries are read frominstalled PE; no guessed runtime callback layout.')
Path('outputs/controller-default-registration-lookup-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified default registration lookup:',153,'bytes; service table:',44,'bytes')
