from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x1097d6e0,0x1097d737')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=87,routines=groups,findings=[
 'Read checker returns false for existing error bit2. Signed maxGet1c compared to raw32 wrapped get0c+request; overflow sets bit2 and returns false.',
 'Resident range uses signed get>=offset20 and capacity4>=wrapped(get-offset+request); otherwise calls callback24(request). False callback ORs live error bit2; true callback returns success.',
 'Does not advance read cursor or recheck range after callback. Negative requests are not rejected by this body.'
],limitations='Overflow callback implementation and production parser integration pending.')
Path('outputs/buffer-read-check-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified buffer read checker:',87,'bytes')
'''
Path('work/recover_buffer_read_check.py').write_text(p)
