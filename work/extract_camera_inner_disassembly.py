from pathlib import Path
lines=Path('work/valve-pyd-camera-disassembly.txt').read_text().splitlines()
for address,count in [('100C0570',125),('100C0420',110),('101BC110',90)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+address+':'))
 section=lines[start:start+count]
 Path('work/native-camera-function-'+address+'.txt').write_text('\n'.join(section)+'\n')
 print('\n'.join(section[-35:]) if address=='100C0570' else '\n'.join(section[:25]))
