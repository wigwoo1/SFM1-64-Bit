from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines();i=next(i for i,s in enumerate(lines) if s.startswith('  1054F280:'));part=lines[i:i+140];Path('work/ifm-float-decompression.txt').write_text('\n'.join(part)+'\n');print('\n'.join(part[30:100]))
