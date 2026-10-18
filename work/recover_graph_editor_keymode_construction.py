import re,struct,json
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');assert im.b[im.offset(0x100a2b20):im.offset(0x100a2b20)+4]==bytes.fromhex('8b410cc3')
spans=[];labels=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x100d9090:break
 if va<0x100d8da0:continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
 if len(raw)==5 and raw[0]==0x68:
  target=struct.unpack('<I',raw[1:])[0]
  try:text=im.string(target)
  except (ValueError,UnicodeError):continue
  if text and all(32<=ord(c)<127 for c in text):labels.append(dict(va=hex(va),string_va=hex(target),text=text))
r=dict(mode_getter=dict(entry='0x100a2b20',bytes='8b410cc3',returns='uint32 at receiver+0x0c; caller receiver=this+0x150, so window mode at+0x15c'),construction_window=dict(begin='0x100d8da0',end='0x100d9090',verified_spans=spans,string_candidates=labels),limitations='Construction strings recovered from bounded region; exact per-action labels/connections require tracing each construction sequence.')
Path('outputs/graph-editor-keymode-construction-recovery.json').write_text(json.dumps(r,indent=2));print(json.dumps(labels,indent=2))
