import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1029aa20,0x1029aad9),(0x1097b170,0x1097b1db)]
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
constants={hex(v):hex(im.word(v)) for v in [0x10aaf280,0x10a8a87c,0x10a99998,0x10aac1ac,0x10a8a56c,0x10ab2848]}
assert constants=={'0x10aaf280':'0x461c4000','0x10a8a87c':'0x3f000000','0x10a99998':'0x80000000','0x10aac1ac':'0x4b000000','0x10a8a56c':'0x3f800000','0x10ab2848':'0x41c00000'}
assert im.string(0x10acfb18)=='frameRate'
Path('outputs/editor-current-time-conversion-recovery.json').write_text(json.dumps(dict(complete_bytes=292,routines=groups,constants=constants,findings=[
 '1029aa20 this228 mode selects integerframe230 using frameRate attribute (default24), rate constructor1097b900 and frame conversion1097ad60. Otherwise override694 uses rawtime698; else1029a950.',
 '1097b170 multiplies inputfloat by10000, adds0.5, SSE bit/sign/round correction then cvttss2si; original fixed integer time units are10000 persecond on this conversion path.',
 'Single precision intermediates and MXCSR state matter; negative half tie goes towardpositive, unlike std::round. NaN/out-of-range cvtt result is80000000.',
 'Rate rational construction/currentbase time/frame conversion still require full recovery before asserting whole playhead behavior.'
],limitations='Complete selected bytes/constants verified; original converter execution parity/MXCSR domain, frame-rate helper and native editor integration pending.'),indent=2)+'\n')
print('Verified current time/conversion bytes:',292)
