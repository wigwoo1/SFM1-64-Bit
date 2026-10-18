from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x1009cbd0,0x1009cc0c')
p=p[:p.index('result=dict(')]+'''vt=0x10aa9978;col=im.word(vt-4);name=im.string(im.word(col+12)+8)
assert name=='.?AVCQSFMController@@' and im.word(col+4)==0x18 and im.word(vt)==0x1009cbd0
result=dict(complete_code_bytes=60,routines=groups,rtti=name,interface_offset=24,findings=[
 'Embedded post-action interface vtable10aa9978 has CQSFMController RTTI and COL offset18; slot0 is1009cbd0.',
 'Callback reads global10f32568 child344; if present calls10275f20(child,1). Writes byte1 at listener-relative219 then RDTSC64 at listener-relative228.',
 'Offsets are relative to embedded interface this; native member identities and child helper semantics remain unresolved.'
],limitations='Constructor connection to concrete registration site, field names and production UI behavior still require verification.')
Path('outputs/controller-undo-listener-callback-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified CQSFMController undo listener callback:',60,'bytes')
'''
Path('work/recover_controller_undo_listener_callback.py').write_text(p)
