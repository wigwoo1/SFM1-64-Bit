from pathlib import Path
lines=Path('work/valve-pyd-camera-disassembly.txt').read_text().splitlines()
start=next(i for i,line in enumerate(lines) if line.startswith('  101BD050:'))
Path('work/native-camera-projection-101BD050.txt').write_text('\n'.join(lines[start:start+220])+'\n')
print('\n'.join(lines[start:start+22]))
