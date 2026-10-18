from pathlib import Path
import struct,json
p=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen.mdl');b=p.read_bytes();slots,families,start=struct.unpack_from('<iii',b,220)
table=[list(struct.unpack_from('<'+'H'*slots,b,start+n*slots*2)) for n in range(families)]
report={'asset':str(p),'skin_slots':slots,'skin_families':families,'skin_tables':table};Path('outputs/sfm64-live-model-skin-metadata.json').write_text(json.dumps(report,indent=2));print(report)
