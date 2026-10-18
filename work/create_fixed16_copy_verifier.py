from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x1097bc10,0x1097bc2d)]').replace('groups=[[],[]]','groups=[[]]')
s=s[:s.index('result=dict')]+'''result=dict(complete_code_bytes=29,routines=groups,findings=[
 '1097bc10 cdecl(src,dst) copies first8bytes before loading/copying second8; unaligned MOVQ, no null/overlap check.',
 '10361c40 passesconstructorarg2 source pointer and element+4 destination, preserving16byte field beforeoriginal14 name field.',
 'Factoryarg4 thus provides16byte source. Identity/UUID meaning inferred from existing DMX16byteIDs but not yet proved by native naming.'
],limitations='Byte verified; exact field semantic naming/original execution parity/native object reconstruction pending.')
Path('outputs/element-fixed16-copy-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified sequential fixed16 copy:',29,'bytes')
'''
Path('work/recover_element_fixed16_copy.py').write_text(s)
