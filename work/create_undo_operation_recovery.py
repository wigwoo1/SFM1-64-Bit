from pathlib import Path
p=Path('work/recover_string_undo_destructor.py').read_text().replace('ranges=[(0x1025eb70,0x1025eb87),(0x102bcc80,0x102bcca2)]','ranges=[(0x10390d50,0x10390e4d)]')
p=p[:p.index('vt=')]+'''result=dict(complete_code_bytes=253,routines=groups,findings=[
 'Begins model slot121(label,1,256), logs, saves flag74 bit3 and disables it.',
 'Uses initial undo count minus1 as loop bound, but pops live undo tail each iteration before record slot0 replay. Appends to redo before reading replayed record marker slot6; any nonzero marker stops group.',
 'Restores only recording bit3 in live flags after end log. If signed field54<=0 clears78/7c. Ends model slot122(0), including empty stack.'
],limitations='Native adapters for notifications, logging, vector growth and replay remain external; runtime integration pending.')
Path('outputs/undo-operation-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo operation:',253,'bytes')
'''
Path('work/recover_undo_operation.py').write_text(p)
