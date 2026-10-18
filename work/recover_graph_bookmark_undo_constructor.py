import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1036c020,0x1036c150)]
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
result=dict(verified_code_bytes=304,routines=groups,label=im.string(0x10b38908),vtable=[hex(im.word(0x10b38f80+4*i)) for i in range(8)],findings=['Calls base10364bd0(attribute,label), installs derived table10b38f80 and stores low byte of fourth argument at2c.', 'Attribute must have maskedtype21 to supply vector data4. Constructor later dereferences this vector without a null guard.', 'New values copied to owned buffer24/count28 before old vector base/count are sampled and copied to owned buffer1c/count20.', 'Positive count with nonnull source requests count*4 bytes, saturating to ffffffff on unsigned multiplication overflow. Buffer initialized to rawtime80000000 then overwritten in source order with live destination reload.', 'Null source or nonpositive count captures empty. Allocation failure is not safely handled: positive copy loop dereferences null destination.'],limitations='Base constructor, allocator binding, derived virtual methods and production undo integration remain pending. Unsafe inputs are documented, not executed.')
Path('outputs/graph-bookmark-undo-constructor-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified bookmark undo constructor:',304,'bytes; label:',result['label'],'; methods:',result['vtable'])
