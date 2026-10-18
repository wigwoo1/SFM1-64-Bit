from pathlib import Path
lines=Path('work/datamodel-pyd-camera-disassembly.txt').read_text().splitlines()
for address,count in [('1029CB60',150),('1029CF60',90)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'))
 section=lines[start:start+count];Path('work/native-camera-wrapper-'+address+'.txt').write_text('\n'.join(section)+'\n')
 print('\n'.join(line for line in section if 'call' in line))
