import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');assert im.word(0x1007c410)==0x1007c3a8;assert im.b[im.offset(0x1007c3ac):im.offset(0x1007c3ac)+5]==bytes.fromhex('e98f1f1200')
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x1019e394:break
 if not (0x1019e340<=va<0x1019e394 or 0x1007c390<=va<0x1007c3b1):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
r=dict(receiver_class='CQTimestripEditor',slot='UpdateHeadLockedButton()',handler='0x1019e340',button_member='0x8c',behavior='If button exists, setChecked(headLocked getter1009bb30), setEnabled(!overwrite getter1009bb40).',imports=json.loads(Path('outputs/head-lock-button-imports.json').read_text()),verified_spans=spans,limitations='Button synchronization recovered; overwrite getter/storage and initial-read/native interaction parity pending.')
Path('outputs/head-lock-button-handler-recovery.json').write_text(json.dumps(r,indent=2));print(r['behavior'])
