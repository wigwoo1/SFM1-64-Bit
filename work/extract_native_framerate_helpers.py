from pathlib import Path
lines=Path('work/valve-pyd-camera-disassembly.txt').read_text().splitlines()
for address,count in [('101C2020',70),('1015EB70',50),('1015EC00',80),('1015F27D',55)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'));section=lines[start:start+count];Path('work/native-time-'+address+'.txt').write_text('\n'.join(section)+'\n');print('\n'.join(section[:40]))
