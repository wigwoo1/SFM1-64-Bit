from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
for a,n in [(0x10508850,110),(0x1054D910,65)]:
 i=next(i for i,s in enumerate(lines) if s.startswith(f'  {a:08X}:'));p=lines[i:i+n];Path(f'work/ifm-empty-curve-{a:08X}.txt').write_text('\n'.join(p)+'\n');print('\n'.join(p[:65]))
