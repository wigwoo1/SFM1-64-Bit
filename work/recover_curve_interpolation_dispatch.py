from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
start=next(i for i,s in enumerate(lines) if s.startswith('  100669D0:'))
Path('work/ifm-curve-interpolation-dispatch.txt').write_text('\n'.join(lines[start:start+700])+'\n')
print('\n'.join(lines[start:start+25]))
