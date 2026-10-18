from pathlib import Path
from pe_image import Image
import re,json
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');lines=[];checked=0;vtables=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m:
   va=int(m[1],16)
   if va>=0x1017cf00:break
   if va>=0x1017cd00:
    b=bytes.fromhex(m[2]);o=im.offset(va);assert im.b[o:o+len(b)]==b;checked+=1;lines.append(line)
    p=re.search(r'mov\s+dword ptr \[.+?\],([0-9A-F]{8})h',line)
    if p:
     pointer=int(p[1],16)
     try:
      col=im.word(pointer-4);name=im.string(im.word(col+12)+8)
      if name.startswith('.?AV'):
       entry=dict(instruction=line.strip(),vtable=hex(pointer),type=name,offset=im.word(col+4))
       if pointer==0x10356a84:entry['selected_clock_slots']={str(i):hex(im.word(pointer+4*i)) for i in (6,7,8,9,12)}
       vtables.append(entry)
     except(ValueError,UnicodeDecodeError):pass
Path('outputs/sfm64-alternate-clock-constructor.txt').write_text(''.join(lines));report=dict(checked_instruction_prefixes=checked,vtables=vtables);Path('outputs/sfm64-alternate-clock-constructor-audit.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2));print(''.join(lines)[:3200])
