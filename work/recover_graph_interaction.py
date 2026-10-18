import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
imports={}
pe=im.u32(60);opt=pe+24;desc=im.base+im.u32(opt+104)
while im.u32(im.offset(desc)):
    original=im.u32(im.offset(desc));dll=im.string(im.base+im.u32(im.offset(desc+12)));iat=im.u32(im.offset(desc+16));n=0
    while True:
        thunk=im.u32(im.offset(im.base+original+n*4))
        if not thunk:break
        if im.base+iat+n*4==0x10a7caf0:
            imports[hex(im.base+iat+n*4)]={'dll':dll,'name':('ordinal '+str(thunk&65535)) if thunk&0x80000000 else im.string(im.base+thunk+2)}
        n+=1
    desc+=20
assert imports['0x10a7caf0']=={'dll':'QtGui4.dll','name':'?restoreOverrideCursor@QApplication@@SAXXZ'}
ranges=[(0x100d34e0,0x100d35ae),(0x100d35b0,0x100d3809)]
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
mode_writes={
    0x100D363C:bytes.fromhex('C7 86 74 01 00 00 01 00 00 00'),
    0x100D3648:bytes.fromhex('C7 86 74 01 00 00 02 00 00 00'),
    0x100D365A:bytes.fromhex('C7 86 74 01 00 00 03 00 00 00'),
    0x100D3666:bytes.fromhex('C7 86 74 01 00 00 00 00 00 00'),
}
for va,expected in mode_writes.items():
    assert im.b[im.offset(va):im.offset(va)+len(expected)]==expected
    assert any(int(row['va'],16)==va and bytes.fromhex(row['bytes'])==expected for row in groups[1])
result=dict(complete_code_bytes=sum(sum(len(bytes.fromhex(s['bytes'])) for s in spans) for spans in groups),routines=groups,imports=imports,mode_write_bytes={hex(va):expected.hex() for va,expected in mode_writes.items()},mode_table=[{'alt':False,'control':False,'mode':0},{'alt':True,'control':False,'mode':2},{'alt':False,'control':True,'mode':3},{'alt':True,'control':True,'mode':1}],findings=['Gesture finalizer 0x100D34E0 reads interaction state at this+0x168. State1 derives a four-int bounds record at this+0x178 from stored coordinates, padding each side by two pixels, then calls 0x100D3250 with the record and state field+0x174.', 'Finalizer states4 and5 dispatch to 0x100C26B0 with the mode object at this+0x150; other states skip that path.', 'Finalizer clears fields+0x168,+0x174,+0x16c, sets byte+0x170, then calls verified QtGui4.dll QApplication::restoreOverrideCursor().', 'Mouse handler0x100D35B0 calls the finalizer on modifier/button transition branches; it snapshots Qt modifier bits25/26/27 and mouse-button bits0/2.', 'Byte-checked stores publish +0x174 modes: Alt+Control=>1, Alt only=>2, Control only=>3, neither=>0. The branch logic uses bits27 and26; Shift bit25 is read but does not select the mode.', 'Cross-verified by outputs/graph-key-bounds-hit-test-verification.json and outputs/graph-key-selection-undo-recovery.json: mode 0 clears prior selection masks before applying hit candidates; mode 1 adds candidates while preserving old selection; mode 2 clears candidate flags and XOR-updates owner selection mask; mode 3 performs that clear/XOR only if the candidate was selected.', 'The separate 0x100C26B0 callback path used by finalizer states4/5 remains unresolved; no Add Key behavior is claimed.'],limitations='Both interaction code spans, mode stores, and the IAT import binding are verified against installed ifm.dll. Hit-test and selection-mode semantics are documented by separate byte-verified receipts. No live SFM gesture or visual result was tested, and the separate mode-state callback 0x100C26B0 remains unresolved.')
Path('outputs/graph-interaction-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified graph interaction:',result['complete_code_bytes'],'bytes; semantic recovery partial')
