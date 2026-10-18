from pathlib import Path
p=Path('work/recover_undo_operation.py').read_text().replace('ranges=[(0x10390d50,0x10390e4d)]','ranges=[(0x1009cce0,0x1009ccfe),(0x100a25e0,0x100a26c8)]')
p=p[:p.index('result=dict(')]+'''strings={hex(v):im.string(v) for v in (0x10aa97d8,0x10aa97d0)}
result=dict(complete_code_bytes=262,routines=groups,strings=strings,findings=[
 'Cleanup builds260-byte path via101b3190(.sfm_autosave.loc) then format%s%s. Formatting wrapper1009cce0 passes varargs to10975800 with capacity260.',
 'Filesystem service subinterface+4 slot14(path,null,buffer260,260,0,0) returns success byte. On success calls main filesystem slot18(path,null), extracts260-byte buffer via1097e660, calls same slot18(secondpath,null).',
 'Always destroys owned temporary buffer and checks security cookie; failure skips both slot18 calls. File operation identities not yet proven.'
],limitations='Path service/filesystem vtable identities and buffer extraction semantics pending; original files were only read, no cleanup executed.')
Path('outputs/controller-autosave-cleanup-recovery.json').write_text(json.dumps(result,indent=2)+'\\n')
print('Verified autosave cleanup/helper:',262,'bytes')
'''
Path('work/recover_controller_autosave_cleanup.py').write_text(p)
