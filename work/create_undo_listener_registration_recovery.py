from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x10390f60,0x10390fe3')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=131,routines=groups,findings=[
 'Listener registration10390f60 scans vector3c/count48 by pointer equality; existing pointer returns false.',
 'Absent pointer appends at original count; grows when count+1 exceeds capacity; refreshes cached base; returns true. Null pointer is not rejected.',
 'Native storage must use pointer-width elements; original scale4 and move byte counts describe x86 storage only.'
],limitations='Growth helper semantics, listener removal/lifetime and production integration pending.')
Path('outputs/undo-listener-registration-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo listener registration:',131,'bytes')
'''
Path('work/recover_undo_listener_registration.py').write_text(p)
