from pathlib import Path
import re
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
index={int(m.group(1),16):i for i,s in enumerate(lines) if (m:=re.match(r'\s*([0-9A-F]{8}):',s))}
out=[]
for a,n in [(0x1038a050,85),(0x1037e240,70),(0x1037e630,75),(0x1037e6d0,95)]:out.extend(lines[index[a]:index[a]+n])
Path('work/ifm-integer-array-undo-execution.txt').write_text('\n'.join(out))
print('\n'.join(out))
