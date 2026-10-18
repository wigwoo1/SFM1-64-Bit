from pathlib import Path
lines=Path('work/datamodel-pyd-camera-disassembly.txt').read_text().splitlines()
for address,count in [('10175880',55),('101759F0',160)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'))
 section=lines[start:start+count];Path('work/native-dme-camera-'+address+'.txt').write_text('\n'.join(section)+'\n');print('\n'.join(section))
