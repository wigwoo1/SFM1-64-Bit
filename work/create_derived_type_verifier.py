from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x1000cb10,0x1000cb37)]').replace('groups=[[],[]]','groups=[[]]')
s=s[:s.index('result=dict')]+'''vtable=0x10a8b0c0
assert im.word(vtable+16)==0x1000cb10
locator=im.word(vtable-4);descriptor=im.word(locator+12)
name=im.string(descriptor+8);assert name=='.?AVCDmeComicSettings@@'
result=dict(complete_code_bytes=39,routines=groups,vtable=hex(vtable),rtti=name,findings=[
 'CDmeComicSettings vslot4 accepts own typeglobal10e7786c, base CDmElement global10e9a684 or wildcardffffffff.',
 'At least this concrete derived implementation preserves base compatibility; this does not prove every serialized class inherits or dispatches identically.'
],limitations='Runtime type registration/global values and other derived classes/native session adapter validation pending.')
Path('outputs/comic-settings-type-validation-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified derived type:',name,'39bytes')
'''
Path('work/recover_comic_settings_type_validation.py').write_text(s)
