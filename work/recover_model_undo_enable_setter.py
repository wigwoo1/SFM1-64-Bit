import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10353bc0,0x10353be1),(0x10390690,0x10390695),(0x103906a0,0x103906a5)]
groups=[[],[],[]];active=None
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
assert im.word(0x10b37094+0xf0)==0x10353bc0
result=dict(complete_code_bytes=43,routines=groups,findings=[
 'Model60 setter10353bc0 testsentireargumentbyte nonzero, notlowbit; delegatesnestedthis+c118 helpers bit3 OR8 orANDf7 atnested74/modelc18c.',
 'Any nonzero byte enablesrecording; zero disables. Other bits includingnotificationsuppressionbit4 preserved.'
],limitations='Completebytesverified; originalexecutionparity/replaycaller toggles/nativeundoqueuebindingpending.')
Path('outputs/model-undo-enable-setter-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo enable setter:',43,'bytes')
