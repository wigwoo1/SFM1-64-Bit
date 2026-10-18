from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x102787d0,0x1027882b')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=91,routines=groups,model_slot97=hex(im.word(0x10b37094+97*4)),findings=[
 '102787d0 resolves child handle80 via model18; missing calls model97 withffffffff.',
 'Present path reloads child handle80 and resolves again; reads second owner field38 and calls model97 with that value. It does not read element name symbol14.',
 'Return value is model97 result. Meaning of model97 and field38 must be traced before naming string filename/session name.'
],limitations='Model97 implementation and production binding pending; second resolution assumes nonnull as original.')
Path('outputs/controller-child-title-source-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified child title source:',91,'bytes; model97',result['model_slot97'])
'''
Path('work/recover_controller_child_title_source.py').write_text(p)
