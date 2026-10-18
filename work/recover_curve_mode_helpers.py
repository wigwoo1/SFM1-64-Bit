from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
addresses=[0x1049AC20,0x1049BA80,0x1049B940,0x1049A500,0x1049AA70,0x1049A7C0,0x10066800]
index={int(s[2:10],16):i for i,s in enumerate(lines) if len(s)>10 and s.startswith('  ') and s[10:11]==':' and all(c in '0123456789ABCDEF' for c in s[2:10])}
for a in addresses:
 i=index[a];Path(f'work/ifm-curve-helper-{a:08X}.txt').write_text('\n'.join(lines[i:i+250])+'\n')
print('extracted helpers:',len(addresses))
