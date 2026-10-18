from pathlib import Path
import json
from audit_pe import inspect
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
paths=[root/'sdktools/python/2.7/win32/Lib/site-packages/vs/valve_pyd.pyd',root/'sdktools/python/2.7/win32/Lib/site-packages/vs/datamodel_pyd.pyd']
results=[]
for p in paths:
 record=inspect(p,root)
 record['camera_strings']=[x.decode('ascii') for x in __import__('re').findall(rb'[ -~]{8,}',p.read_bytes()) if any(k in x for k in [b'GetProjectionMatrix',b'ComputeProjectionMatrix',b'GetFOVx',b'GetViewMatrix'])]
 record.pop('sfm_strings',None)
 results.append(record)
 print(record['path'],'imports',record['imports'],'camera_strings',len(record['camera_strings']))
Path('outputs/sfm64-native-camera-binding-binaries.json').write_text(json.dumps(results,indent=2))
