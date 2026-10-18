from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
start=next(i for i,s in enumerate(lines) if s.startswith('  10066930:'))
part=lines[start:start+100]
Path('work/ifm-curve-kind-decode.txt').write_text('\n'.join(part)+'\n')
print('\n'.join(part[:45]))
