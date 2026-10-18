from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('ranges=[(0x10390d50,0x10390e4d)]','ranges=[(0x10353b50,0x10353b5b),(0x103908b0,0x103908d5)]')
p=p[:p.index('result=dict(')]+'''assert im.word(0x10b37094+126*4)==0x10353b50
result=dict(complete_code_bytes=48,routines=groups,findings=[
 'Model slot126 forwards this+c118 to103908b0.',
 'Snapshots signed listener count48; iterates ascending indices below snapshot count, reloads live listener vector3c each iteration and invokes each listener virtual slot0 without arguments.',
 'No null guard, duplicate filtering or recording-flag gate exists in this hook. Actual listener callback implementations remain external.'
],limitations='Listener registration/lifetime and production UI refresh implementation pending.')
Path('outputs/undo-post-action-listeners-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified undo post-action listeners:',48,'bytes')
'''
Path('work/recover_undo_post_action_listeners.py').write_text(p)
