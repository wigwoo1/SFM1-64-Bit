from pathlib import Path
from pe_image import Image
import hashlib,json
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
assert image.string(0x10EB14FC)=='.?AVCDmeExpressionOperator@@'
locator=image.word(0x10B8E8C0-4)
assert locator==0x10E36BE0 and image.word(locator)==0 and image.word(locator+4)==0
assert image.word(locator+12)==0x10EB14F4
element_locator=image.word(0x10B8E884-4)
assert image.word(element_locator)==0 and image.word(element_locator+4)==4 and image.word(element_locator+12)==0x10EB14F4
slots=[image.word(0x10B8E8C0+4*i) for i in range(8)]
assert slots[:3]==[0x105DEBA0,0x105DEBB0,0x104CB2B0]
expected={0x105DEBA0:'8a4142d0e82401c3',0x105DEBB0:'806142fdc3',0x105DEBC0:'558bec8b45088941605dc20400',0x105DEBD0:'8b4160c3'}
assert image.word(0x10B8E884)==0x101F3E50
expected.update({0x101F3E50:'c20400',0x1035DC00:'8a413ec0e8022401c3',0x1036D0EC:'845e1174168b4e08e8070bffff84c0740a8b4e088b118b0256ffd0'})
for address,data in expected.items():
    offset=image.offset(address);assert image.b[offset:offset+len(bytes.fromhex(data))]==bytes.fromhex(data)
mark_bytes='568bf1f6463e01750f8b461c8b0df4cce91050e878190300804e3e035ec3'
offset=image.offset(0x1035DB90);assert image.b[offset:offset+len(bytes.fromhex(mark_bytes))]==bytes.fromhex(mark_bytes)
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),class_name='CDmeExpressionOperator',
    type_descriptor='0x10eb14f4',complete_object_locator=hex(locator),operator_subobject_offset=0,vtable='0x10b8e8c0',slots=[hex(v) for v in slots],
    dirty_predicate='0x105deba0: return (byte[this+0x42] >> 1) & 1',
    clear_dirty='0x105debb0: byte[this+0x42] &= 0xfd',sort_key_field='this+0x60',
    checked_routine_bytes={hex(k):v for k,v in expected.items()},
    element_subobject_offset=4,element_flag_offset='element+0x3e == operator+0x42',
    element_mark_dirty=dict(routine='0x1035db90',bytes=mark_bytes,behavior='If bit0 clear, enqueue element handle at element+0x1c through static framework global0x10e9ccf4 and helper0x1038f520; then OR flag byte with3.'),
    scalar_write_notification=dict(call_site='0x1036d0ec..0x1036d105',gate='attribute flags0x0100 and owner flags0x04',element_virtual_slot=0,expression_target='0x101f3e50',expression_behavior='ret4: no-op callback'),
    method_names_source='outputs/sfm64-ifm-operator-method-names.json',
    limitations='Installed static class/table/method and owner dirty mark evidence. Attribute mutation gate 0x103623b0 calls this mark helper at0x10362418; recovered separately in attribute_mutation_gate.h. Callback routing and native live activation remain unverified. No serialized DMX attribute is assumed to represent this object flag byte.')
Path('outputs/sfm64-ifm-expression-dirty-state.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified expression operator RTTI, dirty/clear bytes and sort-key accessors')
