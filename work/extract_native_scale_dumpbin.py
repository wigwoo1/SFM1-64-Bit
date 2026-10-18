from pathlib import Path
import json
records=json.loads(Path('outputs/sfm64-native-scale-string-locations.json').read_text())['locations'];out=[]
for record in records:
 name=Path(record['binary']).name
 path=Path('work/datamodel-pyd-camera-disassembly.txt' if name=='datamodel_pyd.pyd' else 'work/native-client-scale-disassembly.txt')
 if name not in ('datamodel_pyd.pyd','client.dll') or not path.exists():continue
 lines=path.read_text(errors='replace').splitlines();needle=f'{record["address"]:08X}'
 for index,line in enumerate(lines):
  if needle in line and not line.lstrip().startswith(needle+':'):
   out.append(dict(binary=record['binary'],string_address=needle,match=line,context=lines[max(0,index-12):index+20],interpretation='dumpbin address text match; native scale composition unverified'))
Path('outputs/sfm64-native-scale-dumpbin-contexts.json').write_text(json.dumps(out,indent=2)+'\n');print('Reference contexts:',len(out))
for record in out:print(Path(record['binary']).name,record['match'])
