import json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
expected=bytes.fromhex('558bec8b4508390584a6e910740b83f8ff740632c05dc20400b0015dc20400')
o=im.offset(0x1035db00);assert im.b[o:o+len(expected)]==expected
assert len(expected)==31 and im.word(0x10b37350)==0x1035db00
factory=json.loads(Path('outputs/control-tree-element-type-recovery.json').read_text())
assert factory['type_global']=='0x10e9a684'
setter=bytes.fromhex(factory['complete_routine_bytes']);o=im.offset(0x10361760);assert im.b[o:o+len(setter)]==setter
vtable=0x10b3737c;locator=im.word(vtable-4);descriptor=im.word(locator+12)
assert im.string(descriptor+8)=='.?AV?$CDmElementFactory@VCDmElement@@@@'
r=dict(base_vtable='0x10b37340',slot4='0x1035db00',complete_bytes=31,routine_bytes=expected.hex(),factory_rtti=im.string(descriptor+8),findings=['Global10e9a684 belongs to CDmElement factory. Base vslot4 accepts token equal global or ffffffff wildcard; rejects all other tokens.','Named-reference getter10002830 passes base CDmElement token, not a renderSettings-specific class token.'],limitations='Derived vslot4 inheritance implementations/runtime token registration and real native session ownership remain unverified. Serialized graph adapter accepts elements as a modeling assumption.')
Path('outputs/base-element-type-validation-recovery.json').write_text(json.dumps(r,indent=2)+'\n')
print('Verified base CDmElement type validation:',31,'bytes; wildcard ffffffff')
