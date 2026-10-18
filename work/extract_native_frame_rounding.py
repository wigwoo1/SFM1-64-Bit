from pathlib import Path
lines=Path('work/valve-pyd-camera-disassembly.txt').read_text().splitlines()
for address,count in [('101570E0',120),('1015EF10',100)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'));section=lines[start:start+count];Path('work/native-time-'+address+'.txt').write_text('\n'.join(section)+'\n');print('\n'.join(section[:65]))
