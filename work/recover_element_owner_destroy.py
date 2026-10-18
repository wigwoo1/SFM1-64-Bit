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
    if va>=0x1035c81c:break
    if va<0x1035c790:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035c790
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035c81c
slots=[i for i in range(160) if im.word(0x10b37094+i*4)==0x1035c790]
Path('outputs/element-owner-destroy-recovery.json').write_text(json.dumps(dict(complete_bytes=140,manager_vtable_slots=slots,instructions=spans,findings=[
 'CDataModel1035c790 validates sentinel, slotcount, generationlow31, inactiveflag and nonnullowner before teardown. Supplies guardedhandle to slot-onlyrelease10356ed0.',
 'If ownerbytec nonzero calls1035b660(handle,1). Then rechecks global10e9a51c and ifnonnull calls1035d9d0(ownerroot8,1).',
 'Calls1025eb70(owner) before releasing poolhandle. Root is read after marked-owner helper; global check also occurs after helper.'
],limitations='Full bytes/vtable slot verified, but marked helper/root cleanup/free contracts and original execution unverified. No interpretation of undocumented service or destructor effects.'),indent=2)+'\n')
print('Verified owner destroy bytes:',140,'manager slots:',slots)
