import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1097ab80,0x1097ac82),(0x1097ad60,0x1097adaa)]
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
table=[im.word(0x1097ac84+4*i) for i in range(4)]
assert table==[0x1097ac12,0x1097abb1,0x1097ab96,0x1097ac7b]
Path('outputs/sfm-frame-to-time-recovery.json').write_text(json.dumps(dict(complete_code_bytes=332,verified_jump_table_bytes=16,routines=groups,jump_table=[hex(v) for v in table],findings=[
 '1097ad60 receives floatframe/packedrate/mode; double(rate.high16)*double(floatframe)/double(rate.low16), passesdouble+mode to1097ab80 and storesreturned32bit.',
 '1097ab80 table mode0 SSEfloor(value+0.5), mode1 SSEfloor(value), mode2 calls10a0efd0 then tail10a0e5b0 (CRT conversion helpers stillpending), mode3 or unsignedmode>3 CVTT truncate.',
 'Floor SSE constants signminuszero/2^52/1 and nearest offset0.5; preserve operation order andMXCSR. Jump table read as data rather than misdisassembledinstructions.',
 'Mode2 candidate uses explicit adapter; stdceil fixture only validates ordinary finite examples, not full CRT/x87 parity.'
],limitations='Code/table verified; mode2 helper semantics/controlstate/originalexecution/currenttime whole integration/native editor pending.'),indent=2)+'\n')
print('Verified frame conversion code/table bytes:',332,16)
