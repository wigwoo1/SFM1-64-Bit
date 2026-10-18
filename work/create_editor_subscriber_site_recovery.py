from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x100a51f7,0x100a520e')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=23,routines=groups,findings=[
 'Concrete call site100a51f7 loads known CDataModel global10e9a51c and invokes slot124 with pointer esi+18.',
 'This passes an embedded listener interface, not the complete owning object. Native port must resolve native interface subobject rather than reuse x86 offset18.',
 'Owner class, embedded vtable and callback implementation are not yet identified; nearby unrelated1f0 accesses are not reliable evidence of model registration.'
],limitations='Concrete subscriber class and callback recovery pending; no UI behavior claimed.')
Path('outputs/editor-post-action-subscriber-site-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified concrete embedded subscriber registration:',23,'bytes')
'''
Path('work/recover_editor_post_action_subscriber_site.py').write_text(p)
