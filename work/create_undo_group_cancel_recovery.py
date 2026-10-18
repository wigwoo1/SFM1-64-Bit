from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x10391130,0x1039118e')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=94,routines=groups,findings=[
 'Disabled recording returns false. Snapshots signed positive counter70 BEFORE cancellation log and group-end call.',
 'Invokes10391080 ignoring return; checks live depth54 afterward. Only zero depth plus saved positive counter invokes undo10390d50 then redo cleanup10390bd0 and returns true.',
 'Cancellation inherits end-group context merging; isolation to only newly submitted records is not proven and must not be imposed on native candidate.'
],limitations='Production runtime binding and cancellation behavior with merged contexts require further validation.')
Path('outputs/undo-group-cancel-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo group cancel:',94,'bytes')
'''
Path('work/recover_undo_group_cancel.py').write_text(p)
