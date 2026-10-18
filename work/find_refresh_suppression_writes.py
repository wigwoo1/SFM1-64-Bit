from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines();out=[]
for i,s in enumerate(lines):
 if 'C198h]' in s and any(op in s for op in ('or          ','and         ','xor         ','mov         byte','mov         dword')):out.extend(lines[max(0,i-12):i+12])
Path('work/ifm-datamodel-suppression-writes.txt').write_text('\n'.join(out));print('\n'.join(out[:200]))
