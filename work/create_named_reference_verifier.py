from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x10002780,0x100027e0),(0x10002830,0x100028b5)]')
s=s[:s.index('result=dict')]+'''result=dict(complete_code_bytes=229,routines=groups,findings=[
 '10002780 initializes static12byte default once: sentinel handle ffffffff at0 and4, zero8. Attribute type flags10 low5==1 returns data pointer4.',
 'Other types call10362e50; true converts via103630a0 into shared scratch10f296fc and returns scratch address; false returns sentinel static.',
 '10002830 separately initializes default static10f29710, looks up name1035dd50, extracts handle through10002780 or default, always resolvesmodel18 including sentinel, then validates resolvedelement vslot4 token10e9a684.',
 'Shared scratch ownership/reentrancy and conversion helpers remain unresolved; do not assume mismatched types simply fail.'
],limitations='Complete bytes verified; conversion helpers/type token initialization/static concurrency/runtime integration pending.')
Path('outputs/named-element-reference-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified typed reference bytes:',229)
'''
Path('work/recover_named_element_reference.py').write_text(s)
