from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for i in (63,64,75,76,126):print(i,hex(im.word(0x10b37094+i*4)))
p=Path('work/recover_undo_operation.py').read_text().replace('ranges=[(0x10390d50,0x10390e4d)]','ranges=[(0x10353d00,0x10353d2f),(0x10353d30,0x10353d5f)]')
p=p[:p.index('result=dict(')]+'''assert im.word(0x10b37094+126*4)==0x1035a5b0
result=dict(complete_code_bytes=94,routines=groups,findings=[
 'Model10353d00/30 checks nested c118 undo/redo availability then performs corresponding operation only if available.',
 'After replay reloads live model vtable and tail calls slot126(1f8). Empty availability returns without slot126. Slot126 target1035a5b0 must be recovered before naming its side effects.'
],limitations='Post-action slot126 semantics and production runtime integration pending.')
Path('outputs/model-undo-redo-actions-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified model undo/redo actions:',94,'bytes')
'''
Path('work/recover_model_undo_redo_actions.py').write_text(p)
