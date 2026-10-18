import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100bf760,0x100bf78f),(0x100bf8c0,0x100bf8e3)]
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
assert im.word(0x10b37094+120*4)==0x10353b90
Path('outputs/notification-suppression-scope-recovery.json').write_text(json.dumps(dict(complete_bytes=82,routines=groups,findings=[
 '100bf760 initializes restoredbyte1=0, saves service slot119 AL into byte0, then invokes slot120 with1.',
 '100bf8c0 checks restoredbyte1; ifzero calls service120 with zeroextended savedbyte0 and only then sets restoredbyte1=1. Repeated restore no-op.',
 'Concrete CDataModel slot120=10353b90 confirmed. Nested scopes therefore preserve earlier suppression if restored in reverse order; no service pointer stored in two-byte scope.',
 'Exact class name, destructor invocation and exception-unwind callers not established; explicit begin/restore candidate avoids assuming automatic lifetime behavior.'
],limitations='Complete bytes/vtable verified; runtime global binding, class identity/caller ownership and original execution parity pending.'),indent=2)+'\n')
print('Verified suppression scope bytes:',82)
