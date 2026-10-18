from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text()
s=s.replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x1028bee0,0x1028bf31)]').replace('groups=[[],[]]','groups=[[]]')
s=s[:s.index('result=dict')]+'''names={hex(v):im.string(v) for v in [0x10a8a6c4,0x10ae44a8]}
result=dict(complete_code_bytes=81,routines=groups,names=names,findings=[
 'Editor field80 raw handle resolved by model18 twice; first null exits, second passed to10002830 using first name.',
 'If first named child missing returnsnull, otherwise10002830 again using second name. No cached primary pointer reused.',
 '10002830 attribute lookup1035dd50, typed handle extraction10002780, model18 resolution and element vslot4 token10e9a684 type validation observed; complete helper still pending.'
],limitations='Getter bytes verified; child helper/type identity/second resolve invalidation/runtime integration pending.')
Path('outputs/editor-rate-object-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified rate-object getter:',81,names)
'''
Path('work/recover_editor_rate_object.py').write_text(s)
