from pathlib import Path
lines=Path('work/valve-pyd-camera-disassembly.txt').read_text().splitlines()
start=next(i for i,line in enumerate(lines) if line.startswith('  101C2A00:'))
section=lines[start:start+160];Path('work/native-rate-101C2A00.txt').write_text('\n'.join(section)+'\n');print('\n'.join(section))
