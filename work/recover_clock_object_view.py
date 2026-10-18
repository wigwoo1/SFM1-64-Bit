from pathlib import Path
from pe_image import Image
import json,struct,re
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll');col=im.word(0x10336cf0);words=[im.word(col+i*4) for i in range(5)];name=im.string(words[3]+8);print('COL',hex(col),[hex(x) for x in words],name)
report=dict(vtable='0x10336cf4',col=hex(col),col_words=[hex(x) for x in words],complete_object_offset=words[1],type_name=name,tick_update_slot1=hex(im.word(0x10336cf8)),tick_history_helper='0x101439d0',limits='RTTI proves secondary view displacement; stateful helper not executed against original.')
Path('outputs/sfm64-animation-clock-object-view-recovery.json').write_text(json.dumps(report,indent=2))
lines=[]
with Path('work/engine-disassembly.txt').open() as f:
 for line in f:
  m=re.match(r'  ([0-9A-F]{8}):',line)
  if m:
   va=int(m[1],16)
   if va>=0x10143bf0:break
   if va>=0x10143adc:lines.append(line)
Path('outputs/sfm64-animation-tick-history-helper-tail.txt').write_text(''.join(lines));print(''.join(lines))
