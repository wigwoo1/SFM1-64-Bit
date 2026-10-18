from pathlib import Path
lines=Path('work/valve-pyd-camera-disassembly.txt').read_text().splitlines()
for address,name in [('100C0740','compute-projection-wrapper'),('100BDA70','perspective-wrapper')]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'))
 section=lines[start:start+300]
 Path('work/native-camera-'+name+'.txt').write_text('\n'.join(section)+'\n')
 print(name)
 for line in section:
  if 'call' in line:print(line)
