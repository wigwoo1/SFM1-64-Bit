from pathlib import Path
from pe_image import Image
from collections import deque
import re,json,struct
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
targets=['10645C70h','106456A0h','10621868h','106285B0h',
         '10629470h','10629630h','10629688h','106295D8h']
history=deque(maxlen=18);lines=[];remaining=0
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  if any(x in line for x in targets):lines.extend(history);remaining=5
  if remaining:lines.append(line);remaining-=1
  history.append(line)
strings=[]
for line in lines:
 m=re.search(r'push\s+([0-9A-F]{8})h',line)
 if m:
  try:strings.append(dict(instruction=line.strip(),text=im.string(int(m[1],16))))
  except (ValueError,UnicodeDecodeError):pass
constants={}
for va,size in [(0x1031bdec,4),(0x1031be7c,4),(0x1031d03c,4),(0x1031dcf8,4),(0x1031e198,8),(0x1031f338,8),(0x1031f3a8,8)]:
 o=im.offset(va);b=im.b[o:o+size];constants[hex(va)]=dict(bytes=b.hex(),value=struct.unpack('<f' if size==4 else '<d',b)[0])
Path('outputs/sfm64-host-clock-configuration.txt').write_text(''.join(lines))
report=dict(strings=strings,constants=constants,command_line_name=im.string(0x10322740))
Path('outputs/sfm64-host-clock-configuration.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
