from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x10335830,0x10335834),(0x10365770,0x10365842)]')
s=s[:s.index('result=dict')]+'''vt=0x10b3820c;col=im.word(vt-4);desc=im.word(col+12)
name=im.string(desc+8);assert name=='.?AV?$CUndoAttributeSetValueElement@VCUtlSymbolLarge@@@@'
assert im.string(0x10b38234)=='CUndoAttributeSetValueElement'
result=dict(complete_code_bytes=214,routines=groups,rtti=name,findings=[
 '10365770 is string-symbol undo record constructor. Model80/81 label calls initialize base metadata8/0c; label4 andstate10zero.',
 'Payload ownerhandle18 viaowner1c getter10335830; oldsymbol1c fromtypedattribute5 orsentineldefault; newsymbol20 fromsourcepointer; attributename symbol14 copiedlast.',
 'All payload fields raw32, distinct nativevtable/labelpointers. Replay helpers and lifecycle not yet recovered.'
],limitations='Completebytes/RTTIverified; model80/81 metadata semantics,undo replay/lifetime/nativeintegrationpending.')
Path('outputs/string-attribute-undo-record-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified string undo constructor/owner handle:',214,'bytes',name)
'''
Path('work/recover_string_attribute_undo_record.py').write_text(s)
