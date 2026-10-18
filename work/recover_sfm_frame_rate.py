import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=0x1097b77d:break
    if va<0x1097b6d0:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1097b6d0
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1097b77d
constants={hex(v):hex(im.word(v)) for v in [0x10a8bd10,0x10a8e010,0x10dda76c,0x10a99c78]}
assert constants=={'0x10a8bd10':'0x7fffffff','0x10a8e010':'0x3a83126f','0x10dda76c':'0x447a4000','0x10a99c78':'0x3c23d70a'}
Path('outputs/sfm-frame-rate-recovery.json').write_text(json.dumps(dict(complete_bytes=173,instructions=spans,constants=constants,findings=[
 '1097b6d0 x87 FISTP rounds input; single precision absolute difference<0.001 stores rounded low16 numerator and10000 units.',
 'Otherwise scale input by1001 then0.001 in separate SSE multiply operations. If abs(roundX87(scaled)-scaled)<0.01 stores roundedscaled low16 numerator and10010 units.',
 'Otherwise rounded original low16 numerator/10000. No gcd reduction or arbitrary rational approximation. Packed low16 numerator/high16 units consumed by1097ad60.',
 'x87 rounding control matters independently of SSE MXCSR. Adapter candidate does not claim default C++ round reproduces all control-word cases.'
],limitations='Complete bytes/constants verified; original x87 execution parity/domain and frame-to-time helper/native editor integration pending.'),indent=2)+'\n')
print('Verified frame rate constructor bytes:',173)
