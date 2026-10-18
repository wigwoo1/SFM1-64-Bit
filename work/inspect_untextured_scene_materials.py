from pathlib import Path
import json
paths=[Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\materials\models\anno\mika\Dick.vmt'),Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\materials\models\anno\chipori\Gem.vmt')]
records=[dict(path=str(p),source=p.read_text(errors='replace')) for p in paths]
Path('outputs/sfm64-untextured-scene-materials.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(records,indent=2))
