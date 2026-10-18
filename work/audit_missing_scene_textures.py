from pathlib import Path
import json
game=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
names=['models/mariogamer/awtter/bodytythoon.vtf','models/mariogamer/dranari/mr_k_Wings and Fluff.vtf']
mounts=sorted(p for p in game.iterdir() if p.is_dir())
records=[]
for name in names:
 exact=[str(m/'materials'/name) for m in mounts if (m/'materials'/name).is_file()]
 basename=Path(name).name.lower();near=[]
 # Search only the relevant asset family; a similarly named file is not a
 # valid replacement without native mounting or material-resolution evidence.
 family=Path(name).parent
 for mount in mounts:
  root=mount/'materials'/family
  if root.is_dir():
   near.extend(str(p) for p in root.rglob('*.vtf'))
 records.append(dict(requested=name,exact_loose_matches=exact,family_texture_files=near))
archives=[str(p) for p in game.glob('*/*.vpk')]
result=dict(textures=records,vpk_archives=archives,limitations='Loose-file exact lookup across immediate game directories. VPK contents and configured mount precedence are not resolved by this audit.')
Path('outputs/sfm64-missing-scene-texture-mount-audit.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(dict(textures=[dict(requested=r['requested'],exact_loose_matches=r['exact_loose_matches'],family_texture_count=len(r['family_texture_files'])) for r in records],vpk_archive_count=len(archives)),indent=2))
