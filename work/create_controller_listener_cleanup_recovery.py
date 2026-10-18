from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x100a2710,0x100a275f')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=79,routines=groups,findings=[
 'Controller cleanup100a2710 removes embedded owner+18 post-action listener via model125 BEFORE imported cleanup calls and optional window slot3(1).',
 'Clears owner window8, invokes100a25e0, then tail calls102f1700 with owner this.',
 'Null owner only maps listener argument to null; later owner dereference means null whole routine is not valid. Routine is cleanup path; destructor caller identity not yet proven.'
],limitations='Imported cleanup APIs, helper semantics and owning destructor caller binding pending.')
Path('outputs/controller-listener-unsubscribe-cleanup-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified controller listener cleanup:',79,'bytes')
'''
Path('work/recover_controller_listener_cleanup.py').write_text(p)
