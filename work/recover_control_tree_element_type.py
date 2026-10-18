import json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
setter=0x10361760
expected=bytes.fromhex('558bec8b4508a384a6e9105dc20400')
at=im.offset(setter)
assert im.b[at:at+len(expected)]==expected
vtable=0x10b3737c
assert im.word(vtable+0x14)==setter
locator=im.word(vtable-4);descriptor=im.word(locator+12)
name=im.string(descriptor+8)
assert name=='.?AV?$CDmElementFactory@VCDmElement@@@@'
receipt=dict(setter=hex(setter),complete_routine_bytes=expected.hex(),
 type_global='0x10e9a684',vtable=hex(vtable),locator=hex(locator),
 descriptor=hex(descriptor),rtti=name,findings=[
 'Population100907d0 checks controls entries against base CDmElement, not a distinct control class.',
 'Transform-specific nine-row branch separately checks CDmeTransformControl global10eb160c.',
 'Nontransform base elements use single kind4 row and shared availability80.'
 ],limitations='Offline setter bytes/factory RTTI verified; original IsA hierarchy/runtime registration still pending.')
Path('outputs/control-tree-element-type-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified base element type:',name,'setter bytes:',len(expected))
