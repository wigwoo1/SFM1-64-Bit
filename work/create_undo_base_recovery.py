from pathlib import Path
p=Path('work/recover_undo_redo_cleanup.py').read_text()
p=p.replace('ranges=[(0x10390bd0,0x10390cdf)]','ranges=[(0x10009310,0x1000931e),(0x10009360,0x10009364),(0x10009370,0x1000937d),(0x10676490,0x10676494)]').replace('groups=[[]]','groups=[[] for _ in ranges]')
p=p[:p.index('result=dict(')]+'''vt=0x10b3820c
slots=[im.word(vt+i*4) for i in range(9)]
assert slots[4:8]==[0x10676490,0x10009310,0x10009360,0x10009370]
result=dict(complete_code_bytes=35,routines=groups,vtable=[hex(x) for x in slots],findings=[
 'String undo slot5 at10009310 null guards this then invokes virtual slot8 with argument1. Actual destructor target captured separately, not recovered here.',
 'Slot4 returns label pointer field4. Slot6 reads raw marker byte10. Slot7 stores low input byte directly, not normalized boolean.'
],limitations='Destructor slot8, retained-data ownership and production runtime integration remain pending.')
Path('outputs/undo-record-base-methods-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo record base methods:',35,'bytes; destructor',hex(slots[8]))
'''
Path('work/recover_undo_record_base_methods.py').write_text(p)
