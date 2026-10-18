from pathlib import Path
s=Path('work/recover_editor_time_adjustment.py').read_text().replace('ranges=[(0x10298ad0,0x10298b31),(0x1097b020,0x1097b085)]','ranges=[(0x10362e50,0x10362e5d),(0x103630a0,0x103630b1)]')
s=s[:s.index('result=dict')]+'''result=dict(complete_code_bytes=30,routines=groups,findings=[
 '10362e50 returns AL=(attribute flags10 low5==1); identical predicate to preceding10002780 direct-data branch.',
 'With stable flags, scratch branch is unreachable for mismatched types. Separate flag reread must be preserved if mutation occurs.',
 '103630a0 copies exactly4bytes from pointed data atattribute4 into supplied output; no parsing/coercion.',
],limitations='Complete bytes verified; static initialization/global type token and native integration pending. No concurrent mutation policy inferred.')
Path('outputs/element-handle-type-check-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified handle type check/copy bytes:',30)
'''
Path('work/recover_element_handle_type_check.py').write_text(s)
p=Path('work/test_named_element_reference.cpp')
t=p.read_text().replace('convertible=false','mutateAfterRead=false')
t=t.replace('unsigned attributeFlags(Attribute* p){return p->flags;}','unsigned attributeFlags(Attribute* p){const auto flags=p->flags;if(mutateAfterRead)p->flags=1;return flags;}')
t=t.replace('return convertible;','return (attr.flags&31)==1;')
t=t.replace('converted.convertible=true;','converted.mutateAfterRead=true;')
p.write_text(t)
