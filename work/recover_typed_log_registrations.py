from pathlib import Path
import re,sys,json
sys.path.insert(0,'work');from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');ls=Path('work/ifm-disassembly.txt').read_text().splitlines();indices={l.strip().split(':')[0]:i for i,l in enumerate(ls) if re.match(r'^\s*[0-9A-F]{8}:',l)};rows=[]
for i,l in enumerate(ls):
 if 'call' not in l or '1038EDB0' not in l:continue
 pushes=[]
 for prev in ls[max(0,i-5):i]:
  m=re.search(r'push\s+([0-9A-F]{8})h',prev)
  if m:pushes.append(int(m[1],16))
 if len(pushes)!=2:continue
 obj,nameaddr=pushes
 try:
  name=im.string(nameaddr);vt=im.word(obj);parent=im.word(vt+0x20);s=indices.get(f'{parent:08X}');body='\n'.join(ls[s:s+18]) if s is not None else ''
  if not name.startswith('Dme') or '10EB34AC' not in body:continue
  setter=im.word(vt+0x14);si=indices.get(f'{setter:08X}');sb='\n'.join(ls[si:si+8]);globalmatch=re.search(r'ds:\[([0-9A-F]{8})h\]',sb)
  rows.append(dict(name=name,factory=hex(obj),vtable=hex(vt),parent_getter=hex(parent),symbol_global='0x'+globalmatch[1] if globalmatch else None))
 except (ValueError,UnicodeError):continue
Path('outputs/sfm-typed-log-registrations.json').write_text(json.dumps(rows,indent=2));print(rows)
Path('work/recovered_log_classes.h').write_text('#pragma once\n#include <string>\n// Registered immediate DmeLog descendants proven by native parent getters.\ninline bool recoveredLogClass(const std::string&type){\n return '+ '||'.join('type=="'+r['name']+'"' for r in rows)+';\n}\n')
