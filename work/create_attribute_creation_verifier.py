from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x103615e0,0x10361701)]').replace('groups=[[],[]]','groups=[[]]')
s=s[:s.index('result=dict')]+'''result=dict(complete_code_bytes=289,routines=groups,findings=[
 '103615e0 queues element1c via1038f520 if state3e bit1 clear, then ORstate3e3 before any duplicate check.',
 'Interns requested name via model50, scans live attribute list30 for symbol14 equality; duplicate returnsnull with preceding marking already performed.',
 'New path savesmodel61/sets60(0), calls10365040(owner,type,name,data), prepends returned attribute without nullcheck, restores10007370.',
 'Optional model62 change record alloc24/init1035e070 then model79; finally propagatesmask3 through1035e400, sets modelc180bit1000, calls10360800(owner,newattribute).'
],limitations='Complete bytes verified; allocation/helper semantics/runtimebinding/full native creation pending; scope60/61 is separate from notification119/120.')
Path('outputs/element-attribute-creation-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified attribute creation body:',289,'bytes')
'''
Path('work/recover_element_attribute_creation.py').write_text(s)
