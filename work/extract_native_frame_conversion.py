from pathlib import Path
lines=Path('work/valve-pyd-camera-disassembly.txt').read_text().splitlines()
for address,count in [('101C2050',120),('1015EF10',100),('101C1F50',100),('1015EC8C',45)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'));section=lines[start:start+count];Path('work/native-time-'+address+'.txt').write_text('\n'.join(section)+'\n')
 if address=='101C2050':print('\n'.join(section[:95]))
