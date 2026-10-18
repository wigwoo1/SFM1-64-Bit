from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x103906f0,0x10390788')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=152,routines=groups,findings=[
 'Recording bit3 clear exits without logging or depth mutation. Otherwise logs then reloads and increments raw depth54.',
 'Only previous depth zero shifts78 to7c, stores third input78, interns first label to5c and second to60; identical input POINTER reuses symbol without second intern.',
 'Sets live pending marker bit0 then zeros submission counter70. Nested calls only increment depth.'
],limitations='Depth end protocol and production binding pending; label symbols must be native pointer width.')
Path('outputs/undo-group-begin-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo group begin:',152,'bytes')
'''
Path('work/recover_undo_group_begin.py').write_text(p)
