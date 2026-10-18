from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x10390ce0,0x10390d4f)]').replace('groups=[[],[]]','groups=[[]]')
s=s[:s.index('result=dict')]+'''result=dict(complete_code_bytes=111,routines=groups,findings=[
 '10390ce0 nullrecord exitswithoutsideeffects; otherwiseincrementsraw32field70 before10390bd0 redo cleanup.',
 'Getsrecordlabelvslot4/loghelper, grows pointervectorwhen signedcount0c>=capacity4, incrementscount then writesrecord atlastslot andupdatescachedbase10.',
 'Ifnestedflags74bit0 set invokesrecordslot7(1) AFTERappend thenclearsbit0 fromliveflags, preservingothermutationbits.'
],limitations='Completebytesverified; redocleanup/fullvectorgrowth/logging/pendingmarker semantics/nativeundostackintegrationpending.')
Path('outputs/undo-record-submission-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo record submission:',111,'bytes')
'''
Path('work/recover_undo_record_submission.py').write_text(s)
