import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x103621b0,0x103621ed)]
groups=[[]];active=None
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
factories=[]
for address in [0x10e9a688,0x10e9a6a4]:
 assert im.word(address)==0x10b3737c
 name=im.string(im.word(address+0x18))
 assert name=='DmElement'
 factories.append(dict(address=hex(address),vtable=hex(im.word(address)),name=name))
assert im.word(0x10b3737c)==0x103621b0
result=dict(complete_code_bytes=61,routines=groups,static_factories=factories,findings=[
 'Two file-backed factory instances useCDmElementFactory vtable and namefield18 points installed DmElement string.',
 'Factoryslot0 allocates original5cbytes via pool10f499f8/1097aa70; nullallocation returnsnull. Successfulcall forwards factory18 serializedname plus callerarguments to10361c40 constructor.',
 'Native candidate must widen pointers/layout; original5c allocation is x86 evidence, not valid x64 size.'
],limitations='Runtime registration of the two instances/callerargument identities/poolnative reconstruction and UI integration pending.')
Path('outputs/base-element-factory-creation-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified DmElement factories and creation:',61,'bytes')
