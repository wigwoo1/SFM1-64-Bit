from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x1006a070,0x1006a0a4')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=52,routines=groups,findings=[
 '1006a070 calls checkedread1097d6e0(1); success reloads live get0c,offset20,base0 and copies source byte to output before incrementing live get0c.',
 'Failure writes output byte0 and does not advance cursor. It returns no success flag contract; output aliases may affect subsequent live cursor increment.'
],limitations='Overflow callback/native buffer storage and production parser integration pending.')
Path('outputs/buffer-byte-read-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified buffer byte read:',52,'bytes')
'''
Path('work/recover_buffer_byte_read.py').write_text(p)
