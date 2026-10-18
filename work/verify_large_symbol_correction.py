from pathlib import Path
p=Path('work/test_string_attribute_update.cpp');s=p.read_text().replace('=11;','=0x10000000bull;').replace('p.symbol==11','p.symbol==0x10000000bull');p.write_text(s)
p=Path('work/test_attribute_initialize.cpp');s=p.read_text().replace('return 7;','return 0x100000007ull;').replace('current->symbol==7','current->symbol==0x100000007ull');p.write_text(s)
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x1035ab09,0x1035ab10),(0x10367690,0x103676d9),(0x103676e0,0x10367729)]').replace('groups=[[],[]]','groups=[[],[],[]]')
s=s[:s.index('result=dict')]+'''assert im.word(0x10b3820c)==0x10367690 and im.word(0x10b38210)==0x103676e0
assert im.string(0x10a8a4f0)==''
result=dict(complete_code_bytes=153,routines=groups,findings=[
 'Interner1035ab09 loadspublishedrecord pointer and adds4 toreturn actualstring address. CUtlSymbolLarge is pointer representation, notnumeric32ID.',
 'Undo/redo vslots0/1 resolveownerhandle18, useattributename14 directlyascharpointer exceptffffffff->empty string, lookup1035dd50,skipnull orflag20, passold1c/new20 bypointer to10366100.',
 'Correction: all native CUtlSymbolLarge fields mustwidentouintptr; 32bit ownerhandles andfixed16data remainunchanged. Olderfixed32-symbol interpretation is superseded.'
],limitations='Original x86 bytes verified; native interner/globaltoken widths elsewhere still requireaudit; replaystack suppression/lifetime/runtimeintegrationpending.')
Path('outputs/string-undo-replay-symbol-width-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo replay and pointer-symbol evidence:',153,'bytes')
'''
Path('work/recover_string_undo_replay_symbol_width.py').write_text(s)
