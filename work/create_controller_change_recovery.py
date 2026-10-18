from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x102bc460,0x102bc516')
p=p[:p.index('result=dict(')]+'''strings={hex(v):im.string(v) for v in (0x10ae7604,0x10ae756c)}
result=dict(complete_code_bytes=182,routines=groups,strings=strings,findings=[
 'Absent child uses installed default title string. Present child calls102787d0 then formats512-byte buffer if input lowbyte nonzero, otherwise passes child string directly; title setter1098bf80 targets controller33c.',
 'Nonzero lowbyte and controller5a5 plus nonnull100013f0 result invoke that object slot9; then optional100a6250 result receives live child344 and full input via1009d0e0.',
 'Title formatting, helper object identities and native runtime binding remain unresolved; lowbyte controls branch while full input is forwarded.'
],limitations='Complete static code verified; helper internals and UI rendering parity pending.')
Path('outputs/controller-child-change-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified controller child-change routine:',182,'bytes',strings)
'''
Path('work/recover_controller_child_change.py').write_text(p)
