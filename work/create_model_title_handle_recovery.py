from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x10356360,0x103563c2')
p=p[:p.index('result=dict(')]+'''assert im.word(0x10b37094+97*4)==0x10356360
result=dict(complete_code_bytes=98,routines=groups,findings=[
 'Model97 validates32bit handle: sentinel,low20 index bound,generation11 match against full low31 slotword,and slot highbit clear; slot payload pointer must be nonnull.',
 'Returns payload first interned string pointer, mappingffffffff to installed emptystring; invalid handle returns null, distinct from valid empty.',
 'Pool is modelc1a0/countc1ac; semantic filename identity still needs payload initialization evidence. Native slot payload and string must be pointer width.'
],limitations='Pool registration and string identity/production integration pending.')
Path('outputs/model-title-string-handle-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified model97 string handle lookup:',98,'bytes')
'''
Path('work/recover_model_title_string_handle.py').write_text(p)
