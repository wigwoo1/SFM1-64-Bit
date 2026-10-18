from pathlib import Path
lines=Path('work/valve-pyd-camera-disassembly.txt').read_text().splitlines()
for address,count in [('1015FAF0',100),('1015DEC0',100),('1015F090',180)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'));section=lines[start:start+count];Path('work/native-time-'+address+'.txt').write_text('\n'.join(section)+'\n')
 if address=='1015FAF0':print('\n'.join(section[:85]))
