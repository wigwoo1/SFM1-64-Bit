import json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
table=0x10b3939c
locator=im.word(table-4);type_descriptor=im.word(locator+12)
name=im.string(type_descriptor+8)
assert name=='.?AVCDmElementFramework@@'
assert im.word(table+8*4)==0x1038f0d0
assert im.word(table+11*4)==0x1038f360
raw=bytes.fromhex('8b81b4000000c3')
assert im.b[im.offset(0x1038f0d0):im.offset(0x1038f0d0)+7]==raw
Path('outputs/element-framework-identity-recovery.json').write_text(json.dumps(dict(vtable=hex(table),rtti=name,phase_getter_slot=8,phase_getter='1038f0d0',phase_getter_complete_bytes=raw.hex(),update_pipeline_slot=11,update_pipeline='1038f360',findings=[
 'Concrete CDmElementFramework vtable slot8 returns rawuint32 phaseb4, the field written by recovered update pipeline slot11.',
 'Framework phase values used by callback gate are therefore consistent with pipeline phases0..6; phase5/6 are rejected by attribute marking when bound to this interface.',
 'Global10e9ccf8 calls slot8 in attribute callback, but concrete runtime global binding still requires interface-registration evidence. Do not assume RTTI alone proves that global instance.'
],limitations='Installed RTTI/vtable/getter bytes verified. Runtime global binding/call timing/phase reset and UI integration unverified.'),indent=2)+'\n')
print('Verified CDmElementFramework RTTI, slots8/11 and7-byte phase getter')
