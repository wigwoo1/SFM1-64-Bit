import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1097dc50,0x1097dcee),(0x1097dcf0,0x1097de37)]
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
result=dict(complete_code_bytes=485,routines=groups,findings=[
 '1097dc50: nonadvancing whitespace lookahead returns absolute supplied offset; text flag and zero initial error required. Checkedread(offset+1) read-error bit2 is cleared before success test.',
 '1097dcf0: nonadvancing token length including terminator; excludes leading text whitespace. Text ends on NUL or bytes9..13/32; binary ends on NUL only.',
 'Scans chunks up to128 bytes. Lookahead checkedread(chunk+offset) return ignored, bit2 cleared; reloads live cursor/max after callback. Unterminated nonempty tail returns bytes+1; empty returns0.',
 'Source pointer is captured once per chunk; source storage is pointer-width in native candidate. Cursor/offset/max remain raw32 fields with signed comparisons.'
],limitations='Static recovery and x64 adapter tests; not original executable oracle or production UI/parser integration. Refills must supply valid readable storage as the original assumes.')
Path('outputs/buffer-string-length-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified whitespace lookahead and string length:',485,'bytes')
