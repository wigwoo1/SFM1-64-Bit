from pathlib import Path
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
out=[]
for i,s in enumerate(lines):
 if '10E9A51C' in s and ('mov         dword ptr ds:' in s or 'mov         ecx,' not in s):
  out.extend(lines[max(0,i-8):i+12])
Path('work/ifm-datamodel-global-writes.txt').write_text('\n'.join(out));print('\n'.join(out[:180]))
