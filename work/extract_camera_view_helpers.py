from pathlib import Path
lines=Path('work/datamodel-pyd-camera-disassembly.txt').read_text().splitlines()
for address,count in [('1008BA00',16),('100775C0',65),('10059750',100),('1029C950',82)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'))
 section=lines[start:start+count];Path('work/native-camera-view-helper-'+address+'.txt').write_text('\n'.join(section)+'\n');print('\n'.join(section[:16]))
