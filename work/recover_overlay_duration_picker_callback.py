import re,json,hashlib
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x10127aa6:break
 if va<0x101278e0:continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),verified_byte_prefix=raw.hex(),instruction=m[3].strip()))
strings={hex(va):im.string(va) for va in [0x10abaf68,0x10abaf40,0x10abaf18,0x10aa5ccc,0x10aa7bd4]}
r=dict(binary=str(Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')),binary_sha256=hashlib.sha256(im.b).hexdigest(),callback='0x101278e0',cancel='ok=false returns without picker/clip operation',accept='Convert entered double to float then time via1097b170; create material picker and receiver carrying document/track/start/time/parent/picker; connect signals and call picker virtualslot10c.',signals=strings,captured_receiver_offsets=dict(document='0x08',track='0x0c',start='0x10',duration='0x14',parent='0x18',picker='0x1c'),verified_instructions=spans,verified_instruction_count=len(spans),verified_byte_count=sum(len(s['verified_byte_prefix'])//2 for s in spans),limitations='Every listed callback instruction byte sequence was compared to the installed DLL. Dialog accepted/rejected signals route to CQVMTPickerDialogHelper slots; accepted handler inserts overlay elements (see overlay-acceptance-body-recovery.json). Picker slot target is 0x101294B0; its exact Qt-visible effect remains unresolved.')
Path('outputs/overlay-duration-picker-callback-recovery.json').write_text(json.dumps(r,indent=2));print(json.dumps(strings,indent=2))
