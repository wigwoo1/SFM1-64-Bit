from pathlib import Path
import re
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):.*\bret\s',line)
 if m and 0x1001fb90<int(m[1],16)<0x10021500:print(line)
