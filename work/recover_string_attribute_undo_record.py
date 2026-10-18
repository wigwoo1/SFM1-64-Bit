import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10335830,0x10335834),(0x10365770,0x10365842)]
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
vt=0x10b3820c;col=im.word(vt-4);desc=im.word(col+12)
name=im.string(desc+8);assert name=='.?AV?$CUndoAttributeSetValueElement@VCUtlSymbolLarge@@@@'
assert im.string(0x10b38234)=='CUndoAttributeSetValueElement'
result=dict(complete_code_bytes=214,routines=groups,rtti=name,findings=[
 '10365770 is string-symbol undo record constructor. Model80/81 label calls initialize base metadata8/0c; label4 andstate10zero.',
 'Payload ownerhandle18 viaowner1c getter10335830; oldsymbol1c fromtypedattribute5 orsentineldefault; newsymbol20 fromsourcepointer; attributename symbol14 copiedlast.',
 'All payload fields raw32, distinct nativevtable/labelpointers. Replay helpers and lifecycle not yet recovered.'
],limitations='Completebytes/RTTIverified; model80/81 metadata semantics,undo replay/lifetime/nativeintegrationpending.')
Path('outputs/string-attribute-undo-record-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified string undo constructor/owner handle:',214,'bytes',name)
