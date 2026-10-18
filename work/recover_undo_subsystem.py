from pathlib import Path
import re
lines=Path('work/ifm-disassembly.txt').read_text().splitlines()
index={int(m.group(1),16):i for i,s in enumerate(lines) if (m:=re.match(r'\s*([0-9A-F]{8}):',s))}
out=[]
for a,n in [(0x10390ce0,140),(0x10391080,130),(0x10391130,130)]:out.extend(lines[index[a]:index[a]+n])
Path('work/ifm-undo-subsystem-record-replay.txt').write_text('\n'.join(out))
print('\n'.join(out))

