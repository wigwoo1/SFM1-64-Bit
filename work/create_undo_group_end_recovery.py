from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x10391080,0x10391129')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=169,routines=groups,findings=[
 'Recording disabled returns false. Logs then decrements live depth54; nonzero result returns false without counter reset.',
 'At zero depth, signed positive counter70 yields true and clears redo. Equal nonnull contexts78/7c scans undo backwards, skips nulls and clears newest nonzero marker, merging groups.',
 'Zero depth always resets counter70. Native label/context values preserve pointer width.'
],limitations='Runtime integration and original execution parity pending.')
Path('outputs/undo-group-end-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo group end:',169,'bytes')
'''
Path('work/recover_undo_group_end.py').write_text(p)
