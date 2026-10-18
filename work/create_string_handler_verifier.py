from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x10361790,0x103617a0),(0x10a6a580,0x10a6a5a5)]')
s=s[:s.index('result=dict')]+'''vt=0x10b37a14;col=im.word(vt-4);desc=im.word(col+12)
name=im.string(desc+8)
assert name=='.?AV?$CDmAttributeOp@VCUtlSymbolLarge@@@@' and im.word(vt+8)==0x10361790
result=dict(complete_code_bytes=53,routines=groups,rtti=name,vtable=hex(vt),findings=[
 '10a6a580 allocates4byte handler, assigns vtable10b37a14 and stores pointer in type5 table10f49be4; allocationfailure writesnull.',
 'Handler RTTI is CDmAttributeOp<CUtlSymbolLarge>, initializer vslot2 at10361790 writes raw32ffffffff into supplieddata.',
 'Base constructor nameattribute uses element14 storage initialized to sentinel before assigning name symbol through10366100.'
],limitations='Original runtime registration/handler allocation native size/updatehelper/shared symbol services and full constructor integration pending.')
Path('outputs/string-attribute-handler-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified string handler registration/initializer:',53,'bytes',name)
'''
Path('work/recover_string_attribute_handler.py').write_text(s)
