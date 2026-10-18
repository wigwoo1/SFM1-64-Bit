from pathlib import Path
p=Path('work/ifm-disassembly.txt')
lines=p.read_text().splitlines()
start=next(i for i,s in enumerate(lines) if s.startswith('  1054D090:'))
end=next(i for i in range(start+1,len(lines)) if lines[i].startswith('  1054D890:'))
Path('work/ifm-float-curve-sampler.txt').write_text('\n'.join(lines[start:end])+'\n')
print('\n'.join(lines[start:start+130]))
