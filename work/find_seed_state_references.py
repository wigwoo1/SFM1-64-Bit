from pathlib import Path
import re
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
for i,r in enumerate(rows):
 addresses=[int(v,16) for v in re.findall(r'(?<![0-9A-F])([0-9A-F]{8})h',r['instruction'])]
 if any(0x10198b40<=v<0x1019fb40 for v in addresses) and not 0x100d5eb0<=int(r['address'],16)<0x100d5fa7:
  print(r['address'],r['instruction'])
