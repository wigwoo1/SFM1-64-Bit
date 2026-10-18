from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x100a4830,0x100a523d')
p=p[:p.index('result=dict(')]+'''vt=0x10aa9978;col=im.word(vt-4)
assert im.string(im.word(col+12)+8)=='.?AVCQSFMController@@' and im.word(col+4)==0x18 and im.word(vt)==0x1009cbd0
result=dict(complete_code_bytes=2573,routines=groups,findings=[
 'Complete constructor100a4830..100a523d installs CQSFMController embedded undo-listener vtable10aa9978 at owner+18; same function registers owner+18 via known model slot124 at100a51f7.',
 'This closes constructor-to-registration-to-callback provenance. Callback fields relative listener219/228 correspond owner231/240; native layout must use actual members.',
 'Constructor initializes owner238 and240 timestamps after registration. Other constructor dependencies remain imported or unrecovered.'
],limitations='Complete bytes verified, not complete constructor semantic implementation; Qt object lifecycle/native UI integration pending.')
Path('outputs/controller-listener-constructor-provenance-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified full controller constructor provenance:',2573,'bytes')
'''
Path('work/recover_controller_listener_constructor_provenance.py').write_text(p)
