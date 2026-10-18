import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x1035ab09,0x1035ab10),(0x10367690,0x103676d9),(0x103676e0,0x10367729)]
groups=[[],[],[]];active=None
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
assert im.word(0x10b3820c)==0x10367690 and im.word(0x10b38210)==0x103676e0
assert im.string(0x10a8a4f0)==''
result=dict(complete_code_bytes=153,routines=groups,findings=[
 'Interner1035ab09 loadspublishedrecord pointer and adds4 toreturn actualstring address. CUtlSymbolLarge is pointer representation, notnumeric32ID.',
 'Undo/redo vslots0/1 resolveownerhandle18, useattributename14 directlyascharpointer exceptffffffff->empty string, lookup1035dd50,skipnull orflag20, passold1c/new20 bypointer to10366100.',
 'Correction: all native CUtlSymbolLarge fields mustwidentouintptr; 32bit ownerhandles andfixed16data remainunchanged. Olderfixed32-symbol interpretation is superseded.'
],limitations='Original x86 bytes verified; native interner/globaltoken widths elsewhere still requireaudit; replaystack suppression/lifetime/runtimeintegrationpending.')
Path('outputs/string-undo-replay-symbol-width-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified undo replay and pointer-symbol evidence:',153,'bytes')
