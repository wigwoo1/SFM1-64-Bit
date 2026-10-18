import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1097da60,0x1097daa2),(0x1097e390,0x1097e409)]
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
result=dict(complete_code_bytes=187,routines=groups,findings=[
 'Owned constructor initializes base=null,capacity=arg2,growSize=arg1; allocates nonzero capacity through allocator slot1. Initializes error/get/put/field18/offset zero,flags=lowbytearg3, preserves field2c except clearing low2bits.',
 'Nonzero capacity and flags bit8 clear: maxGet=ffffffff then1097da60; otherwise maxGet=0. Finally installs read callback10022910 and write callback1097e2c0.',
 '1097da60 signed put>maxGet gate; if flags8 clear and error1 clear calls1097da10(1), writes NUL at live put-offset on success, clears error1 on failure, then sets maxGet=live put.',
 'Constructor initializes terminator before callback fields are installed. Checked-write1097da10 and allocation failure behavior still need recovery before production integration.'
],limitations='Static verified recovery and delegated constructor candidate; not complete CUtlBuffer implementation.')
Path('outputs/buffer-owned-constructor-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified owned buffer constructor/termination:',187,'bytes')
