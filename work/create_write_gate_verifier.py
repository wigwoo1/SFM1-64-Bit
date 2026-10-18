from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x103623b0,0x10362421)]').replace('groups=[[],[]]','groups=[[]]')
s=s[:s.index('result=dict')]+'''result=dict(complete_code_bytes=113,routines=groups,findings=[
 '103623b0 initialflags20 rejectswithoutphasequery. Phase global10e9ccf8 slot8:0/1/2allowed;4allowedonly flags400clear;others reject.',
 'Allowed path rereadsflags; if80clear and2000set invokesowner1035e3a0(attribute). Then ORflags4080 and callsowner1035db90, returnstrue.',
 'Preparation can mutate flags; finalOR is live memory rather than captured flags. Existing marks do not bypass phase gating.'
],limitations='Complete bytes verified; phase semantic names/runtime phasebinding/owner service integration pending.')
Path('outputs/attribute-write-gate-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified attribute write gate:',113,'bytes')
'''
Path('work/recover_attribute_write_gate.py').write_text(s)
