from pathlib import Path
import json,struct,collections
game=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod')
models=json.loads(Path('outputs/sfm64-multimodel-scene-assets.json').read_text())['models']
roots=set()
for model in models:
 relative=Path(model['asset']).parent
 roots.add(game/'materials'/relative)
records=[];counts=collections.Counter()
for root in sorted(roots):
 if not root.is_dir():continue
 for path in sorted(root.rglob('*.vtf')):
  with path.open('rb') as f:header=f.read(80)
  if len(header)<64 or header[:4]!=b'VTF\0':continue
  fmt=struct.unpack_from('<I',header,52)[0];counts[fmt]+=1
  records.append(dict(path=str(path),format=fmt,width=struct.unpack_from('<H',header,16)[0],height=struct.unpack_from('<H',header,18)[0]))
result=dict(scope='VTF headers in matching material directory trees; not exact resolved shader dependency list',format_counts=dict(sorted(counts.items())),textures=records)
Path('outputs/sfm64-scene-texture-formats.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(dict(texture_count=len(records),format_counts=result['format_counts']),indent=2))
