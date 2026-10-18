from pathlib import Path
s=Path('work/recover_restore_blend_helper.py').read_text().replace('begin=0x1002f410','begin=0x1002ebb0')
s=s.replace("context=json.loads(Path('outputs/sfm64-full-restore-dependency-audit.json').read_text())", "context=json.loads(Path('outputs/sfm64-restore-blend-helper-recovery.json').read_text())")
s=s.replace("Path('outputs/sfm64-restore-blend-helper-recovery.json').write_text", "Path('outputs/sfm64-restore-commit-helper-recovery.json').write_text")
s=s.replace("'observed_prefix':'Receiver fields bytefc=0, DWORD100=2,104=1,108=1. First command is opcode1/state27/value0 or device slot57 equivalent. Each following command reacquires command state through1000df50; actual wholefunction semantics still to map.'", "'observed_prefix':'Dirty134 calls1002e490(3); dirty135 calls1002e410(1) then state14/value138. Dirty13c adjusts cached140 bit8 from byte13d. Remaining branches require policy recovery.'")
s=s.replace("'full_restore_callsite':callsite", "'blend_restore_callsite':callsite")
s=s.replace("'callsite':callsite[0]['context']", "'callsite':callsite[0]")
Path('work/recover_restore_commit_helper.py').write_text(s)
