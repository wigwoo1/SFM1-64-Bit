"""Inventory session headers without loading or modifying sessions."""
import json,pathlib,re
root=pathlib.Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\elements\sessions')
records=[]
for p in sorted(root.rglob('*.dmx')):
 with p.open('rb') as f: prefix=f.read(256)
 match=re.match(rb'<!--[^\x00]*?-->',prefix)
 records.append(dict(path=str(p),bytes=p.stat().st_size,header=match.group().decode('ascii') if match else 'unrecognized',prefix_hex=prefix[:16].hex() if not match else None))
out=pathlib.Path(__file__).resolve().parent.parent/'outputs'/'sfm-session-fixtures.json'
out.write_text(json.dumps(records,indent=2))
from collections import Counter
print('Session files:',len(records));print('Headers:',dict(Counter(x['header'] for x in records)))
