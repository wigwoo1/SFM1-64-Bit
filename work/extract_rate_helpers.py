from pathlib import Path
lines=Path('work/valve-pyd-camera-disassembly.txt').read_text().splitlines()
for addr,count in [('1015D6C0',260),('1015D9E0',160),('1015DC60',80)]:
 start=next(i for i,line in enumerate(lines) if line.startswith('  '+addr+':'))
 section=lines[start:start+count];Path('work/native-rate-'+addr+'.txt').write_text('\n'.join(section)+'\n')
 print(addr);print('\n'.join(line for line in section if 'call' in line or '101C' in line))
