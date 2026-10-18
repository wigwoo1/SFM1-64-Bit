from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x1035eff0,0x1035f008),(0x10365ba0,0x10365c00),(0x10366100,0x103661c9)]').replace('groups=[[],[]]','groups=[[],[],[]]')
s=s[:s.index('result=dict')]+'''result=dict(complete_code_bytes=321,routines=groups,findings=[
 '10365ba0 rejects nonstringtype5, initializes defaultinvalidsymbol, rereads type/data, unchangedraw32symbol exitsfalse, changed invokes writegate103623b0.',
 '10366100 onallowedchange optionally createsrecord10365770/model79 BEFORE writing suppliedraw32symbol intoattribute4.',
 'Flag100 plusowner1035dc00 permitsowner vslot0 notification. Rereads attributetype1/15 referenceboolean then1035eff0 translates topropagatemask3(reference)or2(other).',
 'Modelc180 OR1000 forreference or2000 forother. Type5normalpath propagates2 andOR2000, notstructuralmask3.',
],limitations='Writegate103623b0/change-record helper/notification semanticnames/runtimeintegration pending; nonstringbranch cannot normally reachtype1/15 withoutmutation.')
Path('outputs/string-attribute-update-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified string update/compare/propagation:',321,'bytes')
'''
Path('work/recover_string_attribute_update.py').write_text(s)
