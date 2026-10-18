import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x102787d0,0x1027882b)]
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
result=dict(complete_code_bytes=91,routines=groups,model_slot97=hex(im.word(0x10b37094+97*4)),findings=[
 '102787d0 resolves child handle80 via model18; missing calls model97 withffffffff.',
 'Present path reloads child handle80 and resolves again; reads second owner field38 and calls model97 with that value. It does not read element name symbol14.',
 'Return value is model97 result. Meaning of model97 and field38 must be traced before naming string filename/session name.'
],limitations='Model97 implementation and production binding pending; second resolution assumes nonnull as original.')
Path('outputs/controller-child-title-source-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified child title source:',91,'bytes; model97',result['model_slot97'])
