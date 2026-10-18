from pathlib import Path
import re
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
index={int(m.group(1),16):i for i,s in enumerate(lines) if (m:=re.match(r'\s*([0-9A-F]{8}):',s))}
out=[]
for a,n in [(0x1025bf80,75),(0x1035d9c0,95),(0x1035d9d0,95)]:out.extend(lines[index[a]:index[a]+n])
Path('work/ifm-undo-reference-retention.txt').write_text('\n'.join(out))
print('\n'.join(out))

