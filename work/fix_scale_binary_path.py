from pathlib import Path
import json
records=json.loads(Path('outputs/sfm64-native-scale-string-locations.json').read_text())['locations']
path=next(r['binary'] for r in records if Path(r['binary']).name=='client.dll')
p=Path('work/disassemble_scale_binaries.cmd');s=p.read_text().replace(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\bin\client.dll',path);p.write_text(s)
