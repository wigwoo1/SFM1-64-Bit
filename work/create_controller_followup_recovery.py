from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('ranges=[(0x10390d50,0x10390e4d)]','ranges=[(0x1009a210,0x1009a3a2),(0x1009d0e0,0x1009d0e8)]')
p=p[:p.index('result=dict(')]+'''strings={hex(v):im.string(v) for v in (0x10aa8828,0x10aa881c)}
result=dict(complete_code_bytes=410,routines=groups,strings=strings,findings=[
 '1009d0e0 ignores both caller arguments, calls1009a210 with same this, returns discarding8 argument bytes.',
 '1009a210 returns if controller field8 null; otherwise reads current global child344. Present child supplies byte state via10275f10 and title string via102787d0; absent branch supplies zero and installed text.',
 'Calls imported string composition/setter routines and reference-count release operations. Import identities and full string composition semantics remain pending.'
],limitations='Static bytes verified; Qt import identities/native UI update integration pending.')
Path('outputs/controller-followup-update-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified controller follow-up/update:',410,'bytes',strings)
'''
Path('work/recover_controller_followup_update.py').write_text(p)
