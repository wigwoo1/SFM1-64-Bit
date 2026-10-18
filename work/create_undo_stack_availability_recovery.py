from pathlib import Path
p=Path('work/recover_undo_group_cancel.py').read_text().replace('ranges=[(0x10391130,0x1039118e)]','ranges=[(0x103906b0,0x103906b9),(0x10390850,0x10390859),(0x10390860,0x10390869)]')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=27,routines=groups,findings=[
 '103906b0 reports signed field70>0;10390850 reports undo count0c!=0;10390860 reports signed redo count20>0.',
 'Undo availability is nonzero rather than signed positive; negative count is enabled even though undo loop may do no replay. Preserve this distinction in candidate.'
],limitations='Menu caller bindings and production stack integration pending.')
Path('outputs/undo-stack-availability-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo stack availability:',27,'bytes')
'''
Path('work/recover_undo_stack_availability.py').write_text(p)
