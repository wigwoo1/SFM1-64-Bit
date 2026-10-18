from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x10353bf0,0x10353bfc),(0x10353c00,0x10353c2d),(0x10353d70,0x10353d7f)]').replace('groups=[[],[]]','groups=[[],[],[]]')
s=s[:s.index('result=dict')]+'''assert im.word(0x10b37094+0xf4)==0x10353bf0
assert im.word(0x10b37094+0xf8)==0x10353c00
assert im.word(0x10b37094+0x13c)==0x10353d70
result=dict(complete_code_bytes=72,routines=groups,findings=[
 'Model61 getterreturnsmodelc18c bit3. Model62 queriesvirtual61 FIRST, then rejectsnullowner orowner38ffffffff; otherwise recordsallowed.',
 'Model79 submitadjuststhis+c118 andtailcalls10390ce0(record). Actualqueue/null/lifetime behavior pending.',
 'Recordenabled bit3 differsfromnotification suppressionbit4 onsharedmodelbytec18c.'
],limitations='Completebytesverified; enable setter/replay toggles/owner38semantics/undoqueue submission and runtime integration pending.')
Path('outputs/model-undo-record-gate-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo record gate/getter/submit wrapper:',72,'bytes')
'''
Path('work/recover_model_undo_record_gate.py').write_text(s)
