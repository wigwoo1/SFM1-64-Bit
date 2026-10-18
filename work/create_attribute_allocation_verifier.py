from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x10363350,0x103633d6),(0x10365040,0x10365077)]')
s=s[:s.index('result=dict')]+'''result=dict(complete_code_bytes=189,routines=groups,findings=[
 '10365040 zero type returnsnull; otherwise allocoriginal18bytes pool10f49cac, allocationfailure null, constructor10363350(owner,type,name,data).',
 'Constructor stores caller data4 and owner8, interns name->symbol14, stores type low16 into flags10, handle0c ffffffff,next0 null. Originalinputtype exactly1 or15 OR400.',
 'Calls handler table10f49bd0 indexed flagslow5 vslot2 with callerdata; afterwards ORflags200 and returnsattribute. Does not allocate data here.',
],limitations='Handler semantics/dataownership/native allocation layout/valid type range and integration pending.')
Path('outputs/attribute-allocation-constructor-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified attribute allocation/constructor:',189,'bytes')
'''
Path('work/recover_attribute_allocation_constructor.py').write_text(s)
