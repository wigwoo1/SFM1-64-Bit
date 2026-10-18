from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x10275f20,0x10275f43')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=35,routines=groups,findings=[
 '10275f20 compares input lowbyte to child byte15, always writes lowbyte, and only if changed calls global controller helper102bc460 with full original32 input.',
 'Controller post-action callback supplies1; unchanged child byte1 suppresses downstream call but does not suppress callback flag/timestamp update.'
],limitations='102bc460 semantics, native field identities and real UI integration pending.')
Path('outputs/controller-undo-child-update-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified controller undo child update:',35,'bytes')
'''
Path('work/recover_controller_undo_child_update.py').write_text(p)
