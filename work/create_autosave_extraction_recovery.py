from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('0x10390d50,0x10390e4d','0x1097e660,0x1097e76a')
p=p[:p.index('result=dict(')]+'''result=dict(complete_code_bytes=266,routines=groups,findings=[
 '1097e660 initial nonzero error byte14 writes output[0]=0 and returns. Requested capacity0 treated as7fffffff.',
 'Obtains encoded string length via1097dcf0 and optional text whitespace helper1097dbe0. Zero length empties output and sets error bit2.',
 'Copies at most capacity-1 bytes through checked read helper1097d6e0 and null terminates. Oversized branch additionally advances read cursor by length-capacity; nontext path subsequently consumes one byte through1006a070.',
 'No generic standard string trim/line parser is justified without recovering length and whitespace helpers.'
],limitations='Length scanner/whitespace/read helper internals pending; no native parser parity claimed.')
Path('outputs/autosave-buffer-string-extraction-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified autosave buffer extraction:',266,'bytes')
'''
Path('work/recover_autosave_buffer_string_extraction.py').write_text(p)
