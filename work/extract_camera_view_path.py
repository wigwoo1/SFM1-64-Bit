from pathlib import Path
lines=Path('work/datamodel-pyd-camera-disassembly.txt').read_text().splitlines()
for address,count in [('1029CA40',90),('10175100',180)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'))
 section=lines[start:start+count];Path('work/native-camera-view-'+address+'.txt').write_text('\n'.join(section)+'\n');print('\n'.join(section if address=='10175100' else [s for s in section if 'call' in s]))
