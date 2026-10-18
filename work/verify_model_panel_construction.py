from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll');decoded=[]
for line in Path('work/native-client-scale-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if match:decoded.append([int(match[1],16),bytes.fromhex(match[2]),line.strip()])
 elif decoded:
  tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
  if tail:decoded[-1][1]+=bytes.fromhex(tail[1])
checked=[]
for address,data,line in decoded:
 if not(0x101CFD80<=address<0x101CFDB1 or 0x10046DB0<=address<0x10046DE0 or 0x101D2510<=address<0x101D2574):continue
 offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
 checked.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
assert image.b[image.offset(0x101CFD8E):image.offset(0x101CFD8E)+7]==bytes.fromhex('C7 46 04 CC 6E 51 10')
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),setup_routine='0x101cfd80',base_setup='0x10046db0',renderable_vptr_store='0x101cfd8e',renderable_table='0x10516ecc',direct_creation_call='0x101d253c',verified_complete_instructions=checked,limitations='Static setup/creation evidence links the class table to executable construction. Calling panel identity and SFM viewport use remain unverified.')
Path('outputs/sfm64-model-panel-construction.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('Verified construction instructions:',len(checked))
