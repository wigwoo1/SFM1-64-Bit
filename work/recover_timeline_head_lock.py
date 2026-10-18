import re,json,struct
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');ranges=[(0x1009b9b0,0x1009bb30),(0x1009bb30,0x1009bb37),(0x100a6250,0x100a629b),(0x10168dd0,0x10168df1)];spans=[];strings=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x10168df1:break
 if not any(lo<=va<hi for lo,hi in ranges):continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
 if len(raw)==5 and raw[0]==0x68:
  try:text=im.string(struct.unpack('<I',raw[1:])[0])
  except (ValueError,UnicodeError):continue
  if text and all(32<=ord(c)<127 for c in text):strings.append(dict(va=hex(va),text=text))
r=dict(handler='0x10168dd0',behavior='Acquire singleton100a6250; read bytef4 via1009bb30, set logical inverse via1009b9b0.',singleton_global='0x10f2ba04',setter=dict(entry='0x1009b9b0',storage='0xf4',unchanged_value='early return',changed_value='store then UI/notification work'),verified_spans=spans,strings=strings,limitations='Setter secondary UI/notification calls and timeline/playback effect pending; checkbox identity not yet proven.')
Path('outputs/timeline-head-lock-recovery.json').write_text(json.dumps(r,indent=2));print(json.dumps(dict(spans=len(spans),strings=strings),indent=2))
