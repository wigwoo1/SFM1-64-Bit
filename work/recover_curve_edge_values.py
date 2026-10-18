from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
for addr,count in [(0x105089F0,135),(0x10508480,155),(0x105078F0,65)]:
 start=next(i for i,s in enumerate(lines) if s.startswith(f'  {addr:08X}:'))
 p=lines[start:start+count];Path(f'work/ifm-curve-edge-{addr:08X}.txt').write_text('\n'.join(p)+'\n');print('\n'.join(p[:50]))
