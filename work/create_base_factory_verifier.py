from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x103621b0,0x103621ed)]').replace('groups=[[],[]]','groups=[[]]')
s=s[:s.index('result=dict')]+'''factories=[]
for address in [0x10e9a688,0x10e9a6a4]:
 assert im.word(address)==0x10b3737c
 name=im.string(im.word(address+0x18))
 assert name=='DmElement'
 factories.append(dict(address=hex(address),vtable=hex(im.word(address)),name=name))
assert im.word(0x10b3737c)==0x103621b0
result=dict(complete_code_bytes=61,routines=groups,static_factories=factories,findings=[
 'Two file-backed factory instances useCDmElementFactory vtable and namefield18 points installed DmElement string.',
 'Factoryslot0 allocates original5cbytes via pool10f499f8/1097aa70; nullallocation returnsnull. Successfulcall forwards factory18 serializedname plus callerarguments to10361c40 constructor.',
 'Native candidate must widen pointers/layout; original5c allocation is x86 evidence, not valid x64 size.'
],limitations='Runtime registration of the two instances/callerargument identities/poolnative reconstruction and UI integration pending.')
Path('outputs/base-element-factory-creation-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified DmElement factories and creation:',61,'bytes')
'''
Path('work/recover_base_element_factory_creation.py').write_text(s)
