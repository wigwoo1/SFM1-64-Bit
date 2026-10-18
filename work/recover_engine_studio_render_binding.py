from pathlib import Path
import sys,re,json,hashlib
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
assert im.word(0x1040e9b8)==0x103614b4 and im.word(0x1040e9bc)==0x106941d4
assert im.string(0x103614b4)=='VStudioRender026'
assert (0x1040e9b8-0x1040e8e8)//8==26
rows=[];pending=None
for line in Path('work/engine-disassembly.txt').open():
 m=re.match(r'^\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)',line)
 if m:
  va=int(m[1],16);pending=None
  if 0x10239640<=va<0x1023968b:pending={'address':hex(va),'bytes':m[2].strip(),'instruction':m[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+\n?',line):pending['bytes']+=' '+line.strip()
coverage=set()
for r in rows:
 va=int(r['address'],16);b=bytes.fromhex(r['bytes']);assert im.b[im.offset(va):im.offset(va)+len(b)]==b;coverage.update(range(va,va+len(b)))
assert coverage==set(range(0x10239640,0x1023968b))
report={'engine_sha256':hashlib.sha256(im.b).hexdigest(),'interface':'VStudioRender026','table_base':'1040e8e8','entry_index':26,'name_pointer':'103614b4','destination_global':'106941d4','binding':'10239640..8a complete loop byte verified: each8byte entry loads destination+4 intoESI/name+0 intoEAX; skips ifdestination nonnull, otherwise factory(name,null) via[EBX], writesEAX throughESI. Bound1c0h includesentry26. Nonnull results registered for disconnect tracking.','release_connection':'CModelRender100d8220 reads106941d4 andcallsobjectslot34. StudioRender026 registeredfactory/object/vtable separately recovered; its slot34 target100580f0 dispatches10008b60 cleanup. Static binding is now established; actualloadedinstance notobserved.','limitations':'Offline table/consumer proof; runtime factory selection, liveobjectvptr, real resource ownership and renderer integration pending.','instructions':rows}
Path('outputs/sfm64-engine-studio-render-binding.json').write_text(json.dumps(report,indent=2));print('binding verified:',len(rows),'instructions, entry26 -> VStudioRender026/106941d4')
