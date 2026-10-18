from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x100276d0,0x1002772c),(0x103624c0,0x103624e4),(0x10362780,0x10362802)]').replace('groups=[[],[]]','groups=[[],[],[]]')
s=s[:s.index('result=dict')]+'''table=[im.word(0x10362804+4*i) for i in range(6)]
assert table==[0x103627c2,0x103627d7,0x10362799,0x103627fe,0x103627fe,0x103627e5]
constants={hex(v):hex(im.word(v)) for v in [0x10a8a56c,0x10a8dfd4]}
result=dict(complete_code_bytes=258,routines=groups,table=table,constants=constants,findings=[
 '100276d0 direct type3 returns data pointer; other types check103624c0 accepting types2/3/4/7. Unsupported returns staticfloat zero, not caller missingattribute24 fallback.',
 '10362780 type2 signed32 SSE conversion;3 x87floatcopy;4 booleanbyte->0/1;7 signed32 SSEfloat multiplied by installedconstant10a8dfd4. Shared scratch output.',
],limitations='Complete code plus24tablebytes verified; x87specialfloat/controlstate/runtime sharedstorage concurrency and native integration pending.')
Path('outputs/attribute-float-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified float accessor bytes/table/constants:',258,24,constants)
'''
Path('work/recover_attribute_float.py').write_text(s)
