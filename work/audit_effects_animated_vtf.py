from pathlib import Path
import struct,json
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
found=[];scanned=0
for folder in ['tf']:
 for p in (root/folder/'materials'/'effects').rglob('*.vtf'):
  with p.open('rb') as f:b=f.read(80)
  scanned+=1
  if len(b)>=65 and b[:4]==b'VTF\0':
   frames=struct.unpack_from('<H',b,24)[0]
   if frames>1:
    found.append(dict(path=str(p),bytes=p.stat().st_size,version=list(struct.unpack_from('<2I',b,4)),frames=frames,format=struct.unpack_from('<I',b,52)[0],width=struct.unpack_from('<H',b,16)[0],height=struct.unpack_from('<H',b,18)[0],mips=b[56]))
found.sort(key=lambda x:x['bytes'])
Path('outputs/sfm64-effects-animated-vtf-inventory.json').write_text(json.dumps(dict(scanned=scanned,animated=found),indent=2))
print('scanned',scanned,'animated',len(found));print(json.dumps(found[:8],indent=2))

