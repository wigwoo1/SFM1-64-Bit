from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines();i=next(i for i,s in enumerate(lines) if s.startswith('  1049B400:'));print('\n'.join(lines[i:i+12]))
