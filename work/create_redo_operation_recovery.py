from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x10390e50,0x10390f4a').replace('253','250').replace('undo-operation-recovery','redo-operation-recovery')
a=p.index('result=dict(')
p=p[:a]+'''result=dict(complete_code_bytes=250,routines=groups,findings=[
 'Begins model121(label,1,256); logs; saves flag74bit3 and clears it.',
 'Pops live redo tail, invokes recordslot1, appends undo, then tests marker on NEXT remaining redo tail. Stops before consuming that next marked record; unlike undo tests replayed record.',
 'Checks live redo count again after marker callback. Restores bit3 preserving other live flags, conditionally clears78/7c for field54<=0, ends model122(0) even when empty.'
],limitations='Vector growth, notifications, logging and production editor integration remain external adapters.')
Path('outputs/redo-operation-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified redo operation:',250,'bytes')
'''
Path('work/recover_redo_operation.py').write_text(p)
