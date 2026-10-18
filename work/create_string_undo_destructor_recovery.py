from pathlib import Path
p=Path('work/recover_undo_record_base_methods.py').read_text()
p=p.replace('ranges=[(0x10009310,0x1000931e),(0x10009360,0x10009364),(0x10009370,0x1000937d),(0x10676490,0x10676494)]','ranges=[(0x1025eb70,0x1025eb87),(0x102bcc80,0x102bcca2)]')
p=p[:p.index('result=dict(')]+'''assert slots[8]==0x102bcc80
result=dict(complete_code_bytes=57,routines=groups,vtable=[hex(x) for x in slots],findings=[
 'String undo destructor102bcc80 writes base vtable10a8a9fc unconditionally; frees record through1025eb70 iff argument low bit1 is set; returns original pointer.',
 'No payload owner or old/new interned string release calls occur in this complete destructor.',
 '1025eb70 forwards record pointer to allocator global10a7e484 indirect object virtual slot5.'
],limitations='Allocator object binding and interned string pool lifetime remain external; native runtime integration pending.')
Path('outputs/string-undo-destructor-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified string undo destructor and free dispatch:',57,'bytes')
'''
Path('work/recover_string_undo_destructor.py').write_text(p)
