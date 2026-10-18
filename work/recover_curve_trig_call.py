from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
i=next(i for i,s in enumerate(lines) if s.startswith('  10A0F932:'));p=lines[i:i+70];Path('work/ifm-curve-trig-call.txt').write_text('\n'.join(p)+'\n');print('\n'.join(p[:35]))
