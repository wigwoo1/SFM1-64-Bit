from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x1035adc0,0x1035addb)]').replace('groups=[[],[]]','groups=[[]]')
s=s[:s.index('result=dict')]+'''assert im.word(0x10b37094+0xc8)==0x1035adc0
assert im.string(0x10a8e744)=='name'
result=dict(complete_code_bytes=27,routines=groups,findings=[
 'CDataModel vslot50 wrapper1035adc0 forwards caller output pointer/string to1035a9d0 with model+68 manager, returns output pointer.',
 '10361c40 invokes service first with factoryname toelement34 then withconstructorarg3 into stack symbol; name attribute creation103615e0 uses installed name string,type5 andelement14 data, storesattribute pointer18.',
 '10366100 writes stacksymbol into nameattribute via update logic; ORattribute flag400 then fixed16copy completes constructor. Full update helper still pending.',
 'Symbols remain raw32; pointer widening applies to name attribute pointer rather than symbol values.'
],limitations='Full interner/attribute creation/update composition, runtime model binding and native object layout pending.')
Path('outputs/element-constructor-string-service-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified constructor string service:',27,'bytes; attribute name/type5')
'''
Path('work/recover_element_constructor_string_service.py').write_text(s)
