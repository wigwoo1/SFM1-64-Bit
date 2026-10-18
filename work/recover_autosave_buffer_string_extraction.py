import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1097e660,0x1097e76a)]
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
result=dict(complete_code_bytes=266,routines=groups,findings=[
 '1097e660 initial nonzero error byte14 writes output[0]=0 and returns. Requested capacity0 treated as7fffffff.',
 'Obtains encoded string length via1097dcf0 and optional text whitespace helper1097dbe0. Zero length empties output and sets error bit2.',
 'Copies at most capacity-1 bytes through checked read helper1097d6e0 and null terminates. Oversized branch additionally advances read cursor by length-capacity; nontext path subsequently consumes one byte through1006a070.',
 'No generic standard string trim/line parser is justified without recovering length and whitespace helpers.'
],limitations='Length scanner/whitespace/read helper internals pending; no native parser parity claimed.')
Path('outputs/autosave-buffer-string-extraction-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified autosave buffer extraction:',266,'bytes')
