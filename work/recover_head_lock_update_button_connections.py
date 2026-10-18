import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll');spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x101a004f:break
 if va<0x101a001c:continue
 raw=bytes.fromhex(m[2]);at=im.offset(va);assert im.b[at:at+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
r=dict(sender='CQSFMController singleton100a6250',receiver='constructor this/esi',receiver_slot=im.string(0x10ac8304),signals=[im.string(0x10ac82ec),im.string(0x10ac82cc)],connection_type=0,connect_import=json.loads(Path('outputs/head-lock-connect-import.json').read_text()),verified_spans=spans,limitations='Two signal connections proven; receiver class/slot body and settings initial-read behavior pending. Prior initialization call1019e950 not assumed to be update slot.')
Path('outputs/head-lock-update-button-connections.json').write_text(json.dumps(r,indent=2));print(json.dumps(dict(slot=r['receiver_slot'],signals=r['signals']),indent=2))
