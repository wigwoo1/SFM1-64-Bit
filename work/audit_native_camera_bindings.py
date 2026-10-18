from pathlib import Path
import hashlib,json,re
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vs')
result={'scope':'Installed generated bindings; interface signatures only, not implementation parity','sources':[]}
for filename,terms in [('movieobjects.py',['GetViewMatrix','GetProjectionMatrix','GetFOVx','GetFOVy','GetViewProjectionInverse']),('mathlib.py',['ComputeProjectionMatrix','MatrixBuildPerspective'])]:
 p=root/filename;b=p.read_bytes();s=b.decode('utf-8',errors='replace');lines=s.splitlines();matches=[]
 for i,line in enumerate(lines):
  if any(re.search(r'\bdef '+term+r'\b',line) for term in terms):matches.append({'line':i+1,'snippet':'\n'.join(lines[i:i+9])})
 result['sources'].append({'path':str(p),'sha256':hashlib.sha256(b).hexdigest(),'interfaces':matches})
Path('outputs/sfm64-native-camera-binding-evidence.json').write_text(json.dumps(result,indent=2))
print('preserved_interfaces',sum(len(x['interfaces']) for x in result['sources']))
