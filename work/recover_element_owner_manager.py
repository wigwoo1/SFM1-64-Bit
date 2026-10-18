"""Verify owner metadata accessors and concrete manager RTTI."""
import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
table=0x10b37094
assert im.word(table+99*4)==0x10356480
locator=im.word(table-4)
name=im.string(im.word(locator+12)+8)
assert name=='.?AVCDataModel@@'
assert im.string(0x10a8a4f0)==''
ranges=[(0x10356480,0x103564e3),(0x103567b0,0x10356809)]
cursors=[a for a,b in ranges];instructions=[[],[]]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=ranges[-1][1]:break
    for i,(a,b) in enumerate(ranges):
        if not a<=va<b:continue
        raw=bytes.fromhex(m[2]);assert va==cursors[i]
        assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
        cursors[i]+=len(raw)
        instructions[i].append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
assert cursors==[b for a,b in ranges]
report=dict(manager_vtable=hex(table),rtti=name,verified_complete_routines=instructions,
 findings=[
 'Owner pool lookup accessor10356480 belongs to CDataModel vtable slot99.',
 '10356480 resolves owner generational pool exactly as registration; invalid owner returns null. Valid owner object+4 returned as string address unless FFFFFFFF then static empty string10a8a4f0.',
 '103567b0 returns owner object+10 uint32 count, invalid owner0. No side effects.',
 'Owner record has string-bearing metadata and registration count; file association is plausible but not proven by these accessors alone.'
 ],limitations='Complete99-byte string accessor and89-byte count accessor verified, concrete manager RTTI/table binding verified. Slot names, pool object class, file load binding, string lifetime and original runtime execution parity pending.')
Path('outputs/element-owner-manager-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified owner manager:',name,'complete accessor bytes:',sum(b-a for a,b in ranges))
